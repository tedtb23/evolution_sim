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
    if(neighborsVec.size() > maxNeighbors) neighborsVec.erase(neighborsVec.begin() + maxNeighbors, neighborsVec.end());
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
        for(const QuadTreeObject& currObject : objects) {
            if(
                object.id != currObject.id &&
                rangeIsNearRect(object.boundingBox, currObject.boundingBox)
            ) {
                const Vec2 currDistance = getMinDistanceBetweenRects(object.boundingBox, currObject.boundingBox);
                if(currDistance == Vec2(0.0f, 0.0f)) continue;
                if(neighbors.size() >= maxNeighborsInQuad) {
                    for(const QuadTreeObject& neighbor : neighbors) {
                        const Vec2 neighborDistance = getMinDistanceBetweenRects(object.boundingBox, neighbor.boundingBox);
                        if(currDistance < neighborDistance) {
                            neighbors.erase(neighbor);
                            neighbors.emplace(currObject.id, currObject.boundingBox);
                            break;
                        }
                    }
                }else {
                    neighbors.emplace(currObject.id, currObject.boundingBox);
                }
            }
        }
    }
    return neighbors;
}

std::vector<std::pair<uint64_t, Vec2>> QuadTree::raycast(const QuadTreeObject& object) const {
    QuadTreeObjectSet raysNeighbors;
    std::vector<std::pair<uint64_t, Vec2>> neighbors;
    const float rayDistance = 300.0f;
    const float rayWidth = 100.0f;
    const float rayHeight = 100.0f;

    std::array<SDL_FRect, directions.size()> rays = getRays(object, rayDistance, rayWidth, rayHeight);

    for(int i = 0; i < directions.size(); i++) {
        QuadTreeObjectSet rayNeighbors = queryInternal(QuadTreeObject(rays[i]));
        raysNeighbors.merge(rayNeighbors);
    }

    neighbors.reserve(raysNeighbors.size());
    std::transform(raysNeighbors.begin(), raysNeighbors.end(), std::back_inserter(neighbors),
        [object](const QuadTreeObject& neighbor) {
            Vec2 minDist = QuadTree::getMinDistanceBetweenRects(object.boundingBox, neighbor.boundingBox);
            return std::make_pair(neighbor.id, minDist);
        }
    );

    std::sort(neighbors.begin(), neighbors.end(),
        [](const std::pair<uint64_t, Vec2>& neighbor1, const std::pair<uint64_t, Vec2>& neighbor2)-> bool{
            //send neighbors with 0 distance to our object (collisions) to the back of the vector, so they
            //don't take up space of actual neighbors.
            if(neighbor1.second == Vec2(0.0f, 0.0f)) {
                return false;
            }else if(neighbor2.second == Vec2(0.0f, 0.0f)) {
                return true;
            }else {
                return neighbor1.second < neighbor2.second;
            }
        }
    );

    if(neighbors.size() > maxNeighbors) neighbors.erase(neighbors.begin() + maxNeighbors, neighbors.end());

    return neighbors;
}

std::array<SDL_FRect, 8> QuadTree::getRays(const QuadTreeObject& object, const float rayDistance, const float rayWidth, const float rayHeight) const {
    std::array<SDL_FRect, directions.size()> rays{};

    for(int i = 0; i < directions.size(); i++) {
        rays[i] = getRay(directions[i], object, rayDistance, rayWidth, rayHeight);
    }

    return rays;
}

SDL_FRect QuadTree::getRay(const Vec2& direction, const QuadTreeObject& object, const float rayDistance, const float rayWidth, const float rayHeight) const{
    float x, y;

    if(direction.x == 0.0f) {
        x = bounds.x;
    }else if(direction.x < 0.0f) {
        x = object.boundingBox.x - rayDistance < bounds.x ? bounds.x : object.boundingBox.x - rayDistance;
    }else {
        x = object.boundingBox.x + rayDistance > (bounds.x + bounds.w) - rayWidth ? (bounds.x + bounds.w) - rayWidth : object.boundingBox.x + rayDistance;
    }

    if(direction.y == 0.0f) {
        y = object.boundingBox.y;
    }else if(direction.y < 0.0f) {
        y = object.boundingBox.y - rayDistance < bounds.y ? bounds.y : object.boundingBox.y - rayDistance;
    }else {
        y = object.boundingBox.y + rayDistance > (bounds.y + bounds.h) - rayHeight ? (bounds.y + bounds.h) - rayHeight : object.boundingBox.y + rayDistance;
    }

    return SDL_FRect{x, y, rayWidth, rayHeight};
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
    return !(range.x - (rect.x  +  rect.w) > isNearDistance ||
             rect.x  - (range.x + range.w) > isNearDistance ||
             range.y - (rect.y  +  rect.h) > isNearDistance ||
             rect.y  - (range.y + range.h) > isNearDistance );
}

Vec2 QuadTree::getMinDistanceBetweenRects(const SDL_FRect& rect, const SDL_FRect& range) {
    float distLeft   = rect.x  - (range.x + range.w);
    float distRight  = range.x - (rect.x  + rect.w );
    float distTop    = rect.y  - (range.y + range.h);
    float distBottom = range.y - (rect.y  + rect.h );
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