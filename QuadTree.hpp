#ifndef QUADTREE_HPP
#define QUADTREE_HPP

#include "SDL3/SDL.h"
#include "UtilityStructs.hpp"
#include <vector>
#include <functional>
#include <memory>
#include <cstdint>
#include <unordered_set>
#include <array>

class QuadTree {
public:
    struct QuadTreeObject {
        uint64_t id;
        SDL_FRect boundingBox;
        bool highPriority;

        //QuadTreeObject(const uint64_t id, const bool setHighPriority = false) :
        //        id(id), boundingBox({0,0,0,0}), highPriority(setHighPriority) {};
        QuadTreeObject(const SDL_FRect& boundingBox, const bool setHighPriority = false) :
            id(UINT64_MAX), boundingBox(boundingBox), highPriority(setHighPriority) {};
        QuadTreeObject(const uint64_t id, const SDL_FRect& boundingBox, const bool setHighPriority = false) :
            id(id), boundingBox(boundingBox), highPriority(setHighPriority) {};

        bool operator==(const QuadTreeObject& other) const {
            return this->id == other.id;
        }
        bool operator<(const QuadTreeObject& other) const {
            return this->id < other.id;
        }
        bool operator>(const QuadTreeObject& other) const {
            return this->id > other.id;
        }
    };
    /**
    * @param bounds an sdl float rectangle with the x,y members pointing to the top left point of the rectangle
    * @param granularity the amount of points that can be in a rectangle before it is subdivided further
    */
    QuadTree(const SDL_FRect& bounds, uint8_t granularity) : bounds(bounds), granularity(granularity) {objects.reserve(granularity);};

    QuadTree(const QuadTree& other) : bounds(other.bounds), objects(other.objects), granularity(other.granularity), divided(other.divided) {
        if(divided) {
            for(int i = 0; i < 4; i++) {
                children[i] = std::make_unique<QuadTree>(*other.children[i]);
            }
        }
    }

    QuadTree& operator=(const QuadTree& other) {
        if (this == &other) return *this;

        bounds = other.bounds;
        objects = other.objects;
        granularity = other.granularity;
        divided = other.divided;

        for(int i = 0; i < 4; i++) {
            if(other.children[i]) {
                children[i] = std::make_unique<QuadTree>(*other.children[i]);
            }else {
                children[i].reset();
            }
        }
        return *this;
    }

    //[[nodiscard]] SDL_FRect getBounds() const{return bounds;}

    void insert(const QuadTreeObject& object);
    void remove(const QuadTreeObject& object);
    void undivide();

    [[nodiscard]] static bool rangeIntersectsRect(const SDL_FRect& rect, const SDL_FRect& range);
    [[nodiscard]] static bool rangeIsNearRect(const SDL_FRect& rect, const SDL_FRect& range);
    [[nodiscard]] static Vec2 getMinDistanceBetweenRects(const SDL_FRect& rect, const SDL_FRect& range);
    [[nodiscard]] std::vector<uint64_t> query(const QuadTreeObject& object) const;
    [[nodiscard]] std::vector<std::pair<uint64_t, Vec2>> getNearestNeighbors(const QuadTreeObject& object) const;
    [[nodiscard]] std::vector<std::pair<uint64_t, Vec2>> raycast(const QuadTreeObject& object, Vec2 velocityCopy) const;
    [[nodiscard]] std::vector<std::pair<uint64_t, uint64_t>> getIntersections() const;

    void show(SDL_Renderer& renderer) const;
    [[nodiscard]] size_t size() const;

private:
    struct QuadTreeObjectHash {
        std::size_t operator()(const QuadTreeObject& object) const {
            return std::hash<uint64_t>()(object.id);
        }
    };

    struct QuadTreeObjectPair {
        QuadTreeObject first;
        QuadTreeObject second;

        QuadTreeObjectPair(const QuadTreeObject& first, const QuadTreeObject& second) : first(first), second(second) {};

        bool operator==(const QuadTreeObjectPair& other) const {
            const auto& thisSmaller = std::min(this->first, this->second);
            const auto& thisLarger = std::max(this->first, this->second);
            const auto& otherSmaller = std::min(other.first, other.second);
            const auto& otherLarger = std::max(other.first, other.second);
            return thisSmaller == otherSmaller && thisLarger == otherLarger;
        }
    };

    struct QuadTreeObjectPairHash {
        std::size_t operator()(const QuadTreeObjectPair& pair) const {
            return std::hash<uint64_t>()(pair.first.id) ^ std::hash<uint64_t>()(pair.second.id);
        }
    };

    SDL_FRect bounds;
    std::vector<QuadTreeObject> objects;
    uint8_t granularity;
    bool divided = false;
    //children are ordered counterclockwise from quad 0 (ne, nw, sw, se);
    std::array<std::unique_ptr<QuadTree>, 4> children{};

    static constexpr float minWidth = 10.0f;
    static constexpr float minHeight = 10.0f;
    static constexpr float isNearDistance = 20.0f;
    static constexpr uint8_t maxNeighborsInQuad = 4;
    static constexpr uint8_t maxNeighbors = 8;
    static constexpr std::array<Vec2, 8> directions = {
        Vec2(1.0f, -1.0f), //northeast
        Vec2(0.0f, -1.0f), //north
        Vec2(-1.0f, -1.0f), //northwest
        Vec2(-1.0f, 0.0f), //west
        Vec2(-1.0f, 1.0f), //southwest
        Vec2(0.0f, 1.0f), //south
        Vec2(1.0f, 1.0f), //southeast
        Vec2(1.0f, 0.0f), //east
    };

    using QuadTreeObjectPairSet = std::unordered_set<QuadTreeObjectPair, QuadTreeObjectPairHash>;
    using QuadTreeObjectSet = std::unordered_set<QuadTreeObject, QuadTreeObjectHash>;

    void subdivide();
    std::vector<QuadTreeObject> undivideInternal();
    void insertIntoSubTree(const QuadTreeObject& object);
    void getIntersectionsInternal(QuadTreeObjectPairSet* collisionsPtr) const;
    void queryInternal(const QuadTreeObject& object, QuadTreeObjectSet* collisionsPtr) const;
    void getNearestNeighborsInternal(const QuadTreeObject& object, QuadTreeObjectSet* neighborsPtr) const;
    [[nodiscard]] std::vector<std::pair<uint64_t, Vec2>> raycastInternal(
            const QuadTreeObject& object,
            const Vec2& velocityCopy,
            const float rayDistance) const;
    [[nodiscard]] SDL_FRect getRay(const Vec2& direction, const QuadTreeObject& object, float rayDistance) const;
};
#endif //QUADTREE_HPP
