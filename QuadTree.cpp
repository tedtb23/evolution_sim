#include <memory>
#include <vector>
#include <unordered_set>
#include <cstdint>
#include <algorithm>
#include <sstream>
#include "SDL3/SDL.h"
#include "QuadTree.hpp"
#include "UtilityStructs.hpp"

/**
 * Traverses the QuadTree and returns the amount of elements present in all leaf nodes.
 */
size_t QuadTree::size() const{
    size_t size = 0;
    if(divided) {
        for(const auto& childPtr : children) {
            size += childPtr->size();
        }
    }else {
        size += objects.size();
    }
    return size;
}


void QuadTree::insert(const QuadTreeObject& object) {
    if(!rangeIntersectsRect(bounds, object.boundingBox)) return;
    if(divided) {
        insertIntoSubTree(object);
    }else if(objects.size() < granularity || bounds.w / 2.0f <= minWidth || bounds.h / 2.0f <= minHeight) {
        if(std::find(objects.begin(), objects.end(), object) == objects.end()) objects.push_back(object);
    }else {
        divided = true;
        subdivide();
        insertIntoSubTree(object);
        for(const QuadTreeObject& currObject : objects) {
            insertIntoSubTree(currObject);
        }
        objects.clear();
    }
}

void QuadTree::insertIntoSubTree(const QuadTreeObject& object) {
    if(!divided) return;

    for(const auto& childPtr: children) {
        if(rangeIntersectsRect(childPtr->bounds, object.boundingBox)) childPtr->insert(object);
    }
}

void QuadTree::remove(const QuadTreeObject& object) {
    if(!rangeIntersectsRect(bounds, object.boundingBox)) return;

    if(divided) {
        for(const auto& childPtr: children) {
            if(rangeIntersectsRect(childPtr->bounds, object.boundingBox)) {
                childPtr->remove(object);
            }
        }
    }else {
        const auto& found = std::find(objects.begin(), objects.end(), object);
        if(found != objects.end()) objects.erase(found);
    }
}

std::vector<std::pair<uint64_t, uint64_t>> QuadTree::getIntersections() const{
    QuadTreeObjectPairSet collisions = getIntersectionsInternal();
    std::vector<std::pair<uint64_t, uint64_t>> ids;
    ids.reserve(collisions.size());
    std::transform(collisions.begin(), collisions.end(), std::back_inserter(ids),
         [](const QuadTreeObjectPair& pair) {
                        return std::make_pair(pair.first.id, pair.second.id);
                   }
    );
    return ids;
}

std::unordered_set<QuadTree::QuadTreeObjectPair, QuadTree::QuadTreeObjectPairHash> QuadTree::getIntersectionsInternal() const{
    QuadTreeObjectPairSet collisions;

    if(divided) {
        for(const auto & childPtr : children) {
            QuadTreeObjectPairSet childCollisions;
            childCollisions = childPtr->getIntersectionsInternal();
            collisions.merge(childCollisions);
        }
    }else {
        for(int i = 0; i < objects.size(); i++) {
            const QuadTreeObject& currObject = objects[i];
            for(int j = i + 1; j < objects.size(); j++) {
                const QuadTreeObject& otherObject = objects[j];
                if(rangeIntersectsRect(otherObject.boundingBox, currObject.boundingBox))
                    collisions.emplace(currObject, otherObject);
            }
        }
    }
    return collisions;
}

/**
 * Finds the nearest neighbors of the given QuadTreeObject.
 * @param object the QuadTreeObject to find the neighbors of.
 * @return the nearest neighbors of object in a pairing of id and distance to object, sorted by closest distance first.
 */
std::vector<std::pair<uint64_t, Vec2>> QuadTree::getNearestNeighbors(const QuadTreeObject& object) const{
    if(!rangeIntersectsRect(bounds, object.boundingBox)) return {};

    QuadTreeObjectSet neighbors = getNearestNeighborsInternal(object);
    std::vector<std::pair<uint64_t, Vec2>> neighborsVec;
    neighborsVec.reserve(neighbors.size());
    std::transform(neighbors.begin(), neighbors.end(), std::back_inserter(neighborsVec),
        [object](const QuadTreeObject& neighbor) {
            Vec2 minDist = QuadTree::getMinDistanceBetweenRects(object.boundingBox, neighbor.boundingBox);
            return std::make_pair(neighbor.id, minDist);
        }
    );
    std::sort(neighborsVec.begin(), neighborsVec.end(),
        [](const std::pair<uint64_t, Vec2>& neighbor1, const std::pair<uint64_t, Vec2>& neighbor2)-> bool{
           return neighbor1.second < neighbor2.second;
        }
    );
    return neighborsVec;
}

std::unordered_set<QuadTree::QuadTreeObject, QuadTree::QuadTreeObjectHash> QuadTree::getNearestNeighborsInternal(const QuadTreeObject& object) const{
    QuadTreeObjectSet neighbors;

    if(divided) {
        for(const auto& childPtr : children) {
            if(rangeIntersectsRect(childPtr->bounds, object.boundingBox) || rangeIsNearRect(childPtr->bounds, object.boundingBox)) {
                QuadTreeObjectSet childNeighbors;
                childNeighbors = childPtr->getNearestNeighborsInternal(object);
                neighbors.merge(childNeighbors);
            }
        }
    }else {
        for(const QuadTreeObject& currObject : objects)
            if(rangeIsNearRect(currObject.boundingBox, object.boundingBox) &&
                currObject.id != object.id)
                    neighbors.emplace(currObject.id, currObject.boundingBox);
    }
    return neighbors;
}

/**
 * Checks if the given SimObject intersects any other SimObjects present in the QuadTree.
 * @param object a SimObject struct containing the position and id of the object to check.
 * @return a vector holding the id's of simulation objects the range is intersecting.
*/
std::vector<uint64_t> QuadTree::query(const QuadTreeObject& object) const{
    if(!rangeIntersectsRect(bounds, object.boundingBox)) return {};

    QuadTreeObjectSet collisions = queryInternal(object);
    std::vector<uint64_t> ids;
    ids.reserve(collisions.size());
    std::transform(collisions.begin(), collisions.end(), std::back_inserter(ids),
                   [](const QuadTreeObject& object) {
                        return object.id;
                    }
    );
    return ids;
}

std::unordered_set<QuadTree::QuadTreeObject, QuadTree::QuadTreeObjectHash> QuadTree::queryInternal(const QuadTreeObject& object) const{
    QuadTreeObjectSet collisions;

    if(divided) {
        for(const auto& childPtr : children) {
            if(rangeIntersectsRect(childPtr->bounds, object.boundingBox)) {
                QuadTreeObjectSet childCollisions;
                childCollisions = childPtr->queryInternal(object);
                collisions.merge(childCollisions);
            }
        }
    }else {
        for(const auto& [id, boundingBox] : objects)
            if(id != object.id && rangeIntersectsRect(boundingBox, object.boundingBox))
                collisions.emplace(id, boundingBox);
    }
    return collisions;
}


void QuadTree::subdivide() {
    float newWidth = bounds.w / 2.0f;
    float newHeight = bounds.h / 2.0f;

    children[0] = std::make_unique<QuadTree>(((SDL_FRect) {bounds.x + newWidth, bounds.y, newWidth, newHeight}), granularity);
    children[1] = std::make_unique<QuadTree>(((SDL_FRect) {bounds.x, bounds.y, newWidth, newHeight}), granularity);
    children[2] = std::make_unique<QuadTree>(((SDL_FRect) {bounds.x, bounds.y + newHeight, newWidth, newHeight}), granularity);
    children[3] = std::make_unique<QuadTree>(((SDL_FRect) {bounds.x + newWidth, bounds.y + newHeight, newWidth, newHeight}), granularity);
}

/**
 * Checks if the tree from the current instance down needs to be undivided due to insufficient QuadTreeObjects within itself or its children.
 */
void QuadTree::undivide() {
    undivideInternal();
}

std::vector<QuadTree::QuadTreeObject> QuadTree::undivideInternal() {
    if(divided) {
        std::unordered_set<QuadTreeObject, QuadTreeObjectHash> subObjects;
        for(const auto& childPtr : children) {
            const auto temp = childPtr->undivideInternal();
            subObjects.insert(temp.begin(), temp.end());
        }
        if(subObjects.size() < granularity) {
            //objects.clear();
            objects.insert(objects.end(), subObjects.begin(), subObjects.end());
            for(auto& childPtr : children)
                childPtr.reset();
            divided = false;
        }else {
            std::vector<QuadTreeObject> result;
            result.insert(result.end(), subObjects.begin(), subObjects.end());
            return result;
        }
    }
    return objects;
}

bool QuadTree::rangeIntersectsRect(const SDL_FRect& rect, const SDL_FRect& range) {
    return !(range.x > rect.x  +  rect.w  ||
             range.x + range.w <  rect.x  ||
             range.y > rect.y  +  rect.h  ||
             range.y + range.h <  rect.y  );
}

bool QuadTree::rangeIsNearRect(const SDL_FRect& rect, const SDL_FRect& range) {
    return !(range.x - (rect.x  +  rect.w)  > 20  ||
             rect.x  - (range.x +  range.w) > 20  ||
             range.y - (rect.y  +  rect.h)  > 20 ||
             rect.y  - (range.y +  range.h) > 20 );
}

Vec2 QuadTree::getMinDistanceBetweenRects(const SDL_FRect& rect, const SDL_FRect& range) {
    float distLeft = rect.x - (range.x + range.w);
    float distRight = range.x - (rect.x + rect.w);
    float distTop = rect.y - (range.y + range.h);
    float distBottom = range.y - (rect.y + rect.h);
    float distX, distY;
    if(distLeft < 0.0f && distRight < 0.0f) {
        distX = 0.0f;
    }else if(distLeft < 0.0f) {
        distX = distRight;
    }else distX = -distLeft;
    if(distTop < 0.0f && distBottom < 0.0f) {
        distY = 0.0f;
    }else if(distTop < 0.0f) {
        distY = distBottom;
    }else distY = -distTop;

    return {distX, distY};
}

void QuadTree::show(SDL_Renderer& renderer) const{
    SDL_SetRenderDrawColor(&renderer, 255, 0, 0, 255);
    SDL_RenderRect(&renderer, &bounds);
    if(divided) {
        for(const auto& childPtr : children) {
            childPtr->show(renderer);
        }
    }else {
        SDL_RenderDebugText(&renderer, bounds.x, bounds.y, std::to_string(objects.size()).c_str());
    }
}