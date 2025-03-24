#include "QuadTree.hpp"
#include "SimObject.hpp"
#include <memory>
#include <vector>
#include <unordered_set>
#include <cstdint>
#include <algorithm>
#include "SDL3/SDL.h"

void QuadTree::insert(const SimObject& object) {
    if(divided) {
        insertIntoSubTree(object);
    }else if(objects.size() < capacity || bounds.w / 2.0f <= minWidth || bounds.h / 2.0f <= minHeight) {
        if(std::find(objects.begin(), objects.end(), object) == objects.end()) objects.push_back(object);
    }else {
        divided = true;
        subdivide();
        insertIntoSubTree(object);
        for(const SimObject& currObject : objects) {
            insertIntoSubTree(currObject);
        }
        objects.clear();
    }
}

void QuadTree::insertIntoSubTree(const SimObject& object) {
    if(!divided) return;

    for(const auto& child: children) {
        if(rangeIntersectsRect(child->bounds, object.boundingBox)) child->insert(object);
    }
}

void QuadTree::remove(const SimObject& object) {
    if(divided) {
        for(const auto& child: children) {
            if(rangeIntersectsRect(child->bounds, object.boundingBox)) {
                child->remove(object);
            }
        }
    }else {
        const auto& found = std::find(objects.begin(), objects.end(), object);
        if(found != objects.end()) objects.erase(found);
    }
}

std::vector<SimObject> QuadTree::removeInternal(const SimObject& object) {
}

std::vector<std::pair<uint64_t, uint64_t>> QuadTree::getIntersections() const{
    SimObjectPairSet uniqueCollisions = getIntersectionsInternal();
    std::vector<std::pair<uint64_t, uint64_t>> ids;
    ids.reserve(uniqueCollisions.size());
    std::transform(uniqueCollisions.begin(), uniqueCollisions.end(), std::back_inserter(ids),
         [](const SimObjectPair& pair) {
                        return std::make_pair(pair.first.id, pair.second.id);
                   }
    );
    return ids;
}

std::unordered_set<SimObjectPair, SimObjectPairHash> QuadTree::getIntersectionsInternal() const{
    SimObjectPairSet ids;

    if(divided) {
        SimObjectPairSet childIDs;
        for(const auto & child : children) {
            childIDs = child->getIntersectionsInternal();
            ids.merge(childIDs);
        }
    }else {
        for(int i = 0; i < objects.size(); i++) {
            const SimObject& currObject = objects[i];
            for(int j = i + 1; j < objects.size(); j++) {
                const SimObject& otherObject = objects[j];
                if(rangeIntersectsRect(otherObject.boundingBox, currObject.boundingBox))
                    ids.emplace(currObject, otherObject);
            }
        }
    }
    return ids;
}

/**
 * Checks if the given SimObject intersects any other SimObjects present in the QuadTree.
 * @param object a SimObject struct containing the position and id of the object to check.
 * @return a vector holding the id's of simulation objects the range is intersecting.
 */
std::vector<uint64_t> QuadTree::query(const SimObject& object) const{
    std::vector<uint64_t> ids;
    const SDL_FRect& range = object.boundingBox;

    if(!rangeIntersectsRect(bounds, range)) return {};

    if(divided) {
        std::vector<uint64_t> subIDs;
        for(const auto& child : children) {
            if(rangeIntersectsRect(child->bounds, range)) {
                subIDs = child->query(object);
                ids.insert(ids.end(), subIDs.begin(), subIDs.end());
            }
        }
    }else {
        for(const auto& [id, boundingBox] : objects)
            if(id != object.id && rangeIntersectsRect(boundingBox, range))
                ids.push_back(id);
    }
    return ids;
}

void QuadTree::subdivide() {
    float newWidth = bounds.w / 2.0f;
    float newHeight = bounds.h / 2.0f;

    children[0] = std::make_unique<QuadTree>(((SDL_FRect) {bounds.x + newWidth, bounds.y, newWidth, newHeight}), capacity);
    children[1] = std::make_unique<QuadTree>(((SDL_FRect) {bounds.x, bounds.y, newWidth, newHeight}), capacity);
    children[2] = std::make_unique<QuadTree>(((SDL_FRect) {bounds.x, bounds.y + newHeight, newWidth, newHeight}), capacity);
    children[3] = std::make_unique<QuadTree>(((SDL_FRect) {bounds.x + newWidth, bounds.y + newHeight, newWidth, newHeight}), capacity);
}

/**
 * Checks if the tree from the current instance down needs to be undivided due to insufficient SimObjects within itself or its children.
 */
void QuadTree::undivide() { //maybe change function name, it kind of implies it will undivide the tree no matter what.
    undivideInternal();
}

std::vector<SimObject> QuadTree::undivideInternal() {
    std::vector<SimObject> subObjects;

    if(divided) {
        for(const auto& child : children) {
            const auto temp = child->undivideInternal();
            subObjects.insert(subObjects.end(), temp.begin(), temp.end());
        }
        if(subObjects.size() < capacity) {
            objects = subObjects;
            for(auto& child : children)
                child.reset();
            divided = false;
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

void QuadTree::show(SDL_Renderer& renderer) const{
    SDL_SetRenderDrawColor(&renderer, 255, 0, 0, 255);
    SDL_RenderRect(&renderer, &bounds);
    for(const auto& child : children) {
        if(child) child->show(renderer);
    }
}