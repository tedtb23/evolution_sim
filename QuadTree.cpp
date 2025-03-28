#include "QuadTree.hpp"
#include <memory>
#include <vector>
#include <unordered_set>
#include <cstdint>
#include <algorithm>
#include <sstream>
#include "SDL3/SDL.h"

/**
 * Traverses the QuadTree and returns the amount of elements present in all leaf nodes.
 */
size_t QuadTree::size() const{
    size_t size = 0;
    if(divided) {
        for(const auto& child : children) {
            size += child->size();
        }
    }else {
        size += objects.size();
    }
    return size;
}


void QuadTree::insert(const QuadTreeObject& object) {
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

    for(const auto& child: children) {
        if(rangeIntersectsRect(child->bounds, object.boundingBox)) child->insert(object);
    }
}

void QuadTree::remove(const QuadTreeObject& object) {
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

std::vector<std::pair<uint64_t, uint64_t>> QuadTree::getIntersections() const{
    QuadTreeObjectPairSet uniqueCollisions = getIntersectionsInternal();
    std::vector<std::pair<uint64_t, uint64_t>> ids;
    ids.reserve(uniqueCollisions.size());
    std::transform(uniqueCollisions.begin(), uniqueCollisions.end(), std::back_inserter(ids),
         [](const QuadTreeObjectPair& pair) {
                        return std::make_pair(pair.first.id, pair.second.id);
                   }
    );
    return ids;
}

std::unordered_set<QuadTree::QuadTreeObjectPair, QuadTree::QuadTreeObjectPairHash> QuadTree::getIntersectionsInternal() const{
    QuadTreeObjectPairSet ids;

    if(divided) {
        QuadTreeObjectPairSet childIDs;
        for(const auto & child : children) {
            childIDs = child->getIntersectionsInternal();
            ids.merge(childIDs);
        }
    }else {
        for(int i = 0; i < objects.size(); i++) {
            const QuadTreeObject& currObject = objects[i];
            for(int j = i + 1; j < objects.size(); j++) {
                const QuadTreeObject& otherObject = objects[j];
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
std::vector<uint64_t> QuadTree::query(const QuadTreeObject& object) const{
    std::vector<uint64_t> ids;

    if(!rangeIntersectsRect(bounds, object.boundingBox)) return {};

    if(divided) {
        std::vector<uint64_t> subIDs;
        for(const auto& child : children) {
            if(rangeIntersectsRect(child->bounds, object.boundingBox)) {
                subIDs = child->query(object);
                ids.insert(ids.end(), subIDs.begin(), subIDs.end());
            }
        }
    }else {
        for(const auto& [id, boundingBox] : objects)
            if(id != object.id && rangeIntersectsRect(boundingBox, object.boundingBox))
                ids.push_back(id);
    }
    return ids;
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
        for(const auto& child : children) {
            const auto temp = child->undivideInternal();
            subObjects.insert(temp.begin(), temp.end());
        }
        if(subObjects.size() < granularity) {
            //objects.clear();
            objects.insert(objects.end(), subObjects.begin(), subObjects.end());
            for(auto& child : children)
                child.reset();
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

void QuadTree::show(SDL_Renderer& renderer) const{
    SDL_SetRenderDrawColor(&renderer, 255, 0, 0, 255);
    SDL_RenderRect(&renderer, &bounds);
    if(divided) {
        for(const auto& child : children) {
            child->show(renderer);
        }
    }else {
        SDL_RenderDebugText(&renderer, bounds.x, bounds.y, std::to_string(objects.size()).c_str());
    }
}