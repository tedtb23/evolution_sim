#include "QuadTree.hpp"
#include <memory>
#include <vector>
#include <cstdint>
#include "SDL3/SDL.h"

void QuadTree::insert(const SimObject& object) {
    if(divided) {
        insertIntoSubTree(object);
    }else if(objects.size() < capacity || bounds.w / 2.0f <= minWidth || bounds.h / 2.0f <= minHeight) {
        objects.push_back(object);
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

    /**
     *
     * @param range an sdl float rectangle with the x,y members pointing to the top left point of the rectangle
     * @return a vector holding the id's of simulation objects the range is intersecting.
     */
std::vector<uint64_t> QuadTree::query(const SDL_FRect& range) {
    std::vector<uint64_t> ids;

    if(!rangeIntersectsRect(bounds, range)) {
        return {};
    }

    if(divided) {
        std::vector<uint64_t> subIDs;
        if(rangeIntersectsRect(northEastPtr->getBounds(), range)) {
            subIDs = northEastPtr->query(range);
            ids.insert(ids.end(), subIDs.begin(), subIDs.end());
        }
        if(rangeIntersectsRect(northWestPtr->getBounds(), range)) {
            subIDs = northWestPtr->query(range);
            ids.insert(ids.end(), subIDs.begin(), subIDs.end());
        }
        if(rangeIntersectsRect(southWestPtr->getBounds(), range)) {
            subIDs = southWestPtr->query(range);
            ids.insert(ids.end(), subIDs.begin(), subIDs.end());
        }
        if(rangeIntersectsRect(southEastPtr->getBounds(), range)) {
            subIDs = southEastPtr->query(range);
            ids.insert(ids.end(), subIDs.begin(), subIDs.end());
        }
    }else {
        for(const auto& [id, boundingBox] : objects) {
            if(rangeIntersectsRect(boundingBox, range)) {
                ids.push_back(id);
            }
        }
    }
    return ids;
}

void QuadTree::insertIntoSubTree(const SimObject& object) {
    if(!divided) return;

    if(rangeIntersectsRect(northEastPtr->getBounds(), object.boundingBox)) northEastPtr->insert(object);
    if(rangeIntersectsRect(northWestPtr->getBounds(), object.boundingBox)) northWestPtr->insert(object);
    if(rangeIntersectsRect(southWestPtr->getBounds(), object.boundingBox)) southWestPtr->insert(object);
    if(rangeIntersectsRect(southEastPtr->getBounds(), object.boundingBox)) southEastPtr->insert(object);

    //if(northEastPtr && isPointWithin(northEastPtr->getBounds(), point)) {
    //    northEastPtr->insert(object);
    //}else if(northWestPtr && isPointWithin(northWestPtr->getBounds(), point)) {
    //    northWestPtr->insert(object);
    //}else if(southWestPtr && isPointWithin(southWestPtr->getBounds(), point)) {
    //    southWestPtr->insert(object);
    //}else if(southEastPtr && isPointWithin(southEastPtr->getBounds(), point)) {
    //    southEastPtr->insert(object);
    //}
}

void QuadTree::subdivide() {
    float newWidth = bounds.w / 2.0f;
    float newHeight = bounds.h / 2.0f;

    northEastPtr = std::make_unique<QuadTree>(((SDL_FRect) {bounds.x + newWidth, bounds.y, newWidth, newHeight}), capacity);
    northWestPtr = std::make_unique<QuadTree>(((SDL_FRect) {bounds.x, bounds.y, newWidth, newHeight}), capacity);
    southWestPtr = std::make_unique<QuadTree>(((SDL_FRect) {bounds.x, bounds.y + newHeight, newWidth, newHeight}), capacity);
    southEastPtr = std::make_unique<QuadTree>(((SDL_FRect) {bounds.x + newWidth, bounds.y + newHeight, newWidth, newHeight}), capacity);
}

bool QuadTree::isPointWithin(const SDL_FRect& rect, const Vec2& point) {
    return point.x >= rect.x          &&
           point.x <= rect.x + rect.w &&
           point.y >= rect.y          &&
           point.y <= rect.y + rect.h;
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
    if(northEastPtr) northEastPtr->show(renderer);
    if(northWestPtr) northWestPtr->show(renderer);
    if(southWestPtr) southWestPtr->show(renderer);
    if(southEastPtr) southEastPtr->show(renderer);
}