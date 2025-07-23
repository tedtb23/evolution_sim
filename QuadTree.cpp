#include <memory>
#include <vector>
#include <unordered_set>
#include <cstdint>
#include <algorithm>
#include <sstream>
#include <cassert>
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
    }else if(objects.size() < granularity || bounds.w * 0.5f <= minWidth || bounds.h * 0.5f <= minHeight) {
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

std::vector<std::pair<uint64_t, uint64_t>> QuadTree::getIntersections() const {
    QuadTreeObjectPairSet collisions;
    getIntersectionsInternal(&collisions);
    std::vector<std::pair<uint64_t, uint64_t>> ids;
    ids.reserve(collisions.size());
    std::transform(collisions.begin(), collisions.end(), std::back_inserter(ids),
         [](const QuadTreeObjectPair& pair) {
                        return std::make_pair(pair.first.id, pair.second.id);
                   }
    );
    return ids;
}

void QuadTree::getIntersectionsInternal(QuadTreeObjectPairSet* collisionsPtr) const {
    if(divided) {
        for(const auto & childPtr : children) {
            childPtr->getIntersectionsInternal(collisionsPtr);
        }
    }else {
        for(int i = 0; i < objects.size(); i++) {
            const QuadTreeObject& currObject = objects[i];
            for(int j = i + 1; j < objects.size(); j++) {
                const QuadTreeObject& otherObject = objects[j];
                if(rangeIntersectsRect(otherObject.boundingBox, currObject.boundingBox))
                    collisionsPtr->emplace(currObject, otherObject);
            }
        }
    }
}

/**
 * Finds the nearest neighbors of the given QuadTreeObject.
 * @param object the QuadTreeObject to find the neighbors of.
 * @return the nearest neighbors of object in a pairing of id and distance to object, sorted by closest distance first.
 */
std::vector<std::pair<uint64_t, Vec2>> QuadTree::getNearestNeighbors(const QuadTreeObject& object) const{
    if(!rangeIntersectsRect(bounds, object.boundingBox)) return {};

    QuadTreeObjectSet neighbors;
    getNearestNeighborsInternal(object, &neighbors);
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

void QuadTree::getNearestNeighborsInternal(const QuadTreeObject& object, QuadTreeObjectSet* neighborsPtr) const {
    if(divided) {
        for(const auto& childPtr : children) {
            if(rangeIntersectsRect(childPtr->bounds, object.boundingBox) || rangeIsNearRect(childPtr->bounds, object.boundingBox)) {
                childPtr->getNearestNeighborsInternal(object, neighborsPtr);
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
                if(neighborsPtr->size() >= maxNeighborsInQuad) {
                    for(const QuadTreeObject& neighbor : *neighborsPtr) {
                        const Vec2 neighborDistance = getMinDistanceBetweenRects(object.boundingBox, neighbor.boundingBox);
                        if(currDistance < neighborDistance || currObject.highPriority) {
                            neighborsPtr->erase(neighbor);
                            neighborsPtr->emplace(currObject.id, currObject.boundingBox);
                            break;
                        }
                    }
                }else {
                    neighborsPtr->emplace(currObject.id, currObject.boundingBox);
                }
            }
        }
    }
}

std::vector<std::pair<uint64_t, Vec2>> QuadTree::raycast(const QuadTreeObject& object, Vec2 velocityCopy) const {
    const float rayDistance = 400.0f;

    if(std::max(std::abs(velocityCopy.x), std::abs(velocityCopy.y)) == std::abs(velocityCopy.x)) {
        velocityCopy.y = 0.0f;
    }else velocityCopy.x = 0.0f;

    auto closeNeighbors = raycastInternal(object, velocityCopy, rayDistance);
    return closeNeighbors;
}

std::vector<std::pair<uint64_t, Vec2>> QuadTree::raycastInternal(
        const QuadTreeObject& object,
        const Vec2& velocity,
        const float rayDistance) const {
    QuadTreeObjectSet rayCollisions;
    std::vector<std::pair<uint64_t, Vec2>> result;
    std::unordered_map<uint64_t, bool> priorities;

    queryInternal(
            QuadTreeObject(getRay(velocity, object, rayDistance)),
            &rayCollisions);

    result.reserve(rayCollisions.size());
    std::transform(rayCollisions.begin(), rayCollisions.end(), std::back_inserter(result),
        [&object, &priorities](const QuadTreeObject& neighbor) {
            Vec2 minDist = QuadTree::getMinDistanceBetweenRects(object.boundingBox, neighbor.boundingBox);
            priorities[neighbor.id] = neighbor.highPriority;

            return std::make_pair(neighbor.id, minDist);
        }
    );

    std::sort(result.begin(), result.end(),
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

    std::stable_sort(result.begin(), result.end(),
        [&priorities](const std::pair<uint64_t, Vec2>& neighbor1, const std::pair<uint64_t, Vec2>& neighbor2)-> bool{
        if(!priorities.contains(neighbor1.first) || !priorities.contains(neighbor2.first)) return false;
        return priorities.at(neighbor1.first) && !priorities.at(neighbor2.first);
    });

    if(result.size() > maxNeighbors) result.erase(result.begin() + maxNeighbors, result.end());

    return result;
}

SDL_FRect QuadTree::getRay(const Vec2& direction, const QuadTreeObject& object, float rayDistance) const{
    assert(rayDistance > object.boundingBox.w && rayDistance > object.boundingBox.h);
    float x, y;
    float rayWidth = rayDistance, rayHeight = rayDistance;

    if(direction.x == 0.0f) {
        rayWidth *= 0.10f;
        float potentialX = object.boundingBox.x - ((rayWidth * 0.5f) + (object.boundingBox.w * 0.5f));
        x = potentialX < bounds.x ? bounds.x : potentialX;
    }else if(std::signbit(direction.x)) { //left ray
        float potentialX = object.boundingBox.x - rayDistance;
        x = potentialX < bounds.x ? bounds.x : potentialX;
    }else { //right ray
        x = object.boundingBox.x + object.boundingBox.w;
        rayWidth = x + rayDistance > bounds.x + bounds.w ? (bounds.x + bounds.w) - x : rayDistance;
    }

    if(direction.y == 0.0f) {
        rayHeight *= 0.10f;
        float potentialY = object.boundingBox.y - ((rayHeight * 0.5f) + (object.boundingBox.h * 0.5f));
        y = potentialY < bounds.y ? bounds.y : potentialY;
    }else if(std::signbit(direction.y)) { //up ray
        float potentialY = object.boundingBox.y - rayDistance;
        y = potentialY < bounds.y ? bounds.y : potentialY;
    }else { //down ray
        y = object.boundingBox.y + object.boundingBox.h;
        rayHeight = y + rayDistance > bounds.y + bounds.h ? (bounds.y + bounds.h) - y : rayDistance;
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
    QuadTreeObjectSet collisions;
    queryInternal(object, &collisions);
    std::vector<uint64_t> ids;
    ids.reserve(collisions.size());
    std::transform(collisions.begin(), collisions.end(), std::back_inserter(ids),
                [](const QuadTreeObject& object) {
                    return object.id;
                }
    );
    return ids;
}

void QuadTree::queryInternal(const QuadTreeObject& object, QuadTreeObjectSet* collisionsPtr) const{
    if(divided) {
        for(const auto& childPtr : children) {
            if(rangeIntersectsRect(childPtr->bounds, object.boundingBox)) {
                childPtr->queryInternal(object, collisionsPtr);
            }
        }
    }else {
        for(const auto& [id, boundingBox, highPriority] : objects) {
            if(id != object.id && rangeIntersectsRect(boundingBox, object.boundingBox)) {
                collisionsPtr->emplace(id, boundingBox, highPriority);
            }
        }
    }
}


void QuadTree::subdivide() {
    float newWidth = bounds.w * 0.5f;
    float newHeight = bounds.h * 0.5f;

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

void QuadTree::show(SDL_Renderer* rendererPtr) const{
    SDL_SetRenderDrawColor(rendererPtr, 255, 0, 0, 255);
    SDL_RenderRect(rendererPtr, &bounds);
    if(divided) {
        for(const auto& childPtr : children) {
            childPtr->show(rendererPtr);
        }
    }else {
        SDL_RenderDebugText(rendererPtr, bounds.x, bounds.y, std::to_string(objects.size()).c_str());
    }
}