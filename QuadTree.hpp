#ifndef QUADTREE_HPP
#define QUADTREE_HPP

#include <vector>
#include <memory>
#include <cstdint>
#include "SDL3/SDL.h"
#include "SimObject.hpp"
#include "UtilityStructs.hpp"

class QuadTree {
public:
    /**
    * @param bounds an sdl float rectangle with the x,y members pointing to the top left point of the rectangle
    * @param capacity the amount of points that can be in a rectangle before it is subdivided further
    */
    QuadTree(const SDL_FRect& bounds, uint8_t capacity) : bounds(bounds), capacity(capacity) {objects.reserve(capacity);};
    void insert(const SimObject& object);

    [[nodiscard]] SDL_FRect getBounds() const {return bounds;};
    [[nodiscard]] std::vector<uint64_t> query(const SDL_FRect& range);

    void show(SDL_Renderer& renderer)const;

private:
    SDL_FRect bounds;
    std::vector<SimObject> objects;
    uint8_t capacity;
    bool divided = false;
    std::unique_ptr<QuadTree> northEastPtr;
    std::unique_ptr<QuadTree> northWestPtr;
    std::unique_ptr<QuadTree> southWestPtr;
    std::unique_ptr<QuadTree> southEastPtr;

    static constexpr float minWidth = 10.0f;
    static constexpr float minHeight = 10.0f;

    void subdivide();
    void insertIntoSubTree(const SimObject& object);
    static bool isPointWithin(const SDL_FRect& rect, const Vec2& point);
    static bool rangeIntersectsRect(const SDL_FRect& rect, const SDL_FRect& range);
};


#endif //QUADTREE_HPP
