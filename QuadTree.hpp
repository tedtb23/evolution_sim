#ifndef QUADTREE_HPP
#define QUADTREE_HPP

#include <vector>
#include <memory>
#include <cstdint>
#include <unordered_set>
#include <array>
#include "SDL3/SDL.h"
#include "SimObject.hpp"
#include "UtilityStructs.hpp"

class QuadTree {
public:
    friend class QuadTree;
    /**
    * @param bounds an sdl float rectangle with the x,y members pointing to the top left point of the rectangle
    * @param capacity the amount of points that can be in a rectangle before it is subdivided further
    */
    QuadTree(const SDL_FRect& bounds, uint8_t capacity) : bounds(bounds), capacity(capacity) {objects.reserve(capacity);};

    void insert(const SimObject& object);
    void remove(const SimObject& object);
    void undivide();

    [[nodiscard]] std::vector<uint64_t> query(const SimObject& object) const;
    [[nodiscard]] std::vector<std::pair<uint64_t, uint64_t>> getIntersections() const;

    void show(SDL_Renderer& renderer) const;

private:
    SDL_FRect bounds;
    std::vector<SimObject> objects;
    uint8_t capacity;
    bool divided = false;
    //children are ordered counterclockwise from quad 0 (ne, nw, sw, se);
    std::array<std::unique_ptr<QuadTree>, 4> children{};

    static constexpr float minWidth = 10.0f;
    static constexpr float minHeight = 10.0f;

    using SimObjectPairSet = std::unordered_set<SimObjectPair, SimObjectPairHash>;

    void subdivide();
    std::vector<SimObject> undivideInternal();
    void insertIntoSubTree(const SimObject& object);
    std::vector<SimObject> removeInternal(const SimObject& object);
    static bool rangeIntersectsRect(const SDL_FRect& rect, const SDL_FRect& range);
    [[nodiscard]] std::unordered_set<SimObjectPair, SimObjectPairHash> getIntersectionsInternal() const;
};


#endif //QUADTREE_HPP
