#ifndef QUADTREE_HPP
#define QUADTREE_HPP

#include <vector>
#include <memory>
#include <cstdint>
#include <unordered_set>
#include <array>
#include "SDL3/SDL.h"

class QuadTree {
public:
    struct QuadTreeObject {
        uint64_t id;
        SDL_FRect boundingBox;

        QuadTreeObject(const uint64_t id, const SDL_FRect& boundingBox) : id(id), boundingBox(boundingBox) {};

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
    friend class QuadTree;
    /**
    * @param bounds an sdl float rectangle with the x,y members pointing to the top left point of the rectangle
    * @param granularity the amount of points that can be in a rectangle before it is subdivided further
    */
    QuadTree(const SDL_FRect& bounds, uint8_t granularity) : bounds(bounds), granularity(granularity) {objects.reserve(granularity);};

    void insert(const QuadTreeObject& object);
    void remove(const QuadTreeObject& object);
    void undivide();

    [[nodiscard]] std::vector<uint64_t> query(const QuadTreeObject& object) const;
    [[nodiscard]] std::vector<std::pair<uint64_t, uint64_t>> getIntersections() const;

    void show(SDL_Renderer& renderer) const;
    size_t size() const;

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

    using QuadTreeObjectPairSet = std::unordered_set<QuadTreeObjectPair, QuadTreeObjectPairHash>;

    void subdivide();
    std::vector<QuadTreeObject> undivideInternal();
    void insertIntoSubTree(const QuadTreeObject& object);
    static bool rangeIntersectsRect(const SDL_FRect& rect, const SDL_FRect& range);
    [[nodiscard]] std::unordered_set<QuadTreeObjectPair, QuadTreeObjectPairHash> getIntersectionsInternal() const;
};


#endif //QUADTREE_HPP
