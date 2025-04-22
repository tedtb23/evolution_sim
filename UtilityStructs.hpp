#ifndef UTILITYSTRUCTS_HPP
#define UTILITYSTRUCTS_HPP

#include <cmath>
#include <functional>

struct Vec2 {
    float x;
    float y;
    float gridSize = 10.0f;

    constexpr Vec2(const float x, const float y) : x(x), y(y) {};
    constexpr Vec2(const float x, const float y, const float gridSize) : x(x), y(y), gridSize(gridSize) {};

    /**
     * @return the length/magnitude of the vector from the origin.
     */
    [[nodiscard]] float getDistanceToOrigin() const {
        return static_cast<float>(sqrt((x*x) + (y*y)));
    }

    /**
     * @return the normalized version of the vector
     */
    [[nodiscard]] Vec2 getNormalizedVector() const {
        const float magnitude = getDistanceToOrigin();
        return {x / magnitude, y / magnitude};
    }

    bool operator==(const Vec2& other) const = default;
    bool operator<(const Vec2& other) const{
        return getDistanceToOrigin() < other.getDistanceToOrigin();
    }
    bool operator>(const Vec2& other) const{
        return getDistanceToOrigin() > other.getDistanceToOrigin();
    }
};

struct Vec2PositionalEqual {
    bool operator()(const Vec2& position, const Vec2& otherPosition) const {
        if(position.x < 0.0f || position.y < 0.0f) return false;
        const auto positionCellX = static_cast<uint64_t>(std::floor(position.x / position.gridSize));
        const auto positionCellY = static_cast<uint64_t>(std::floor(position.y / position.gridSize));
        const auto otherPositionCellX = static_cast<uint64_t>(std::floor(otherPosition.x / position.gridSize));
        const auto otherPositionCellY = static_cast<uint64_t>(std::floor(otherPosition.y / position.gridSize));
        return positionCellX == otherPositionCellX && positionCellY == otherPositionCellY;
    }
};

struct Vec2PositionalHash {
    std::size_t operator()(const Vec2& position) const {
        if(position.x < 0.0f || position.y < 0.0f) return 0;
        const auto cellX = static_cast<uint64_t>(std::floor(position.x / position.gridSize));
        const auto cellY = static_cast<uint64_t>(std::floor(position.y / position.gridSize));
        const std::size_t hashX = std::hash<uint64_t>()(cellX);
        const std::size_t hashY = std::hash<uint64_t>()(cellY);
        return hashX ^ (hashY << 1);
    }
};

struct ThreadData {
    std::function<void()> threadFunc;

    explicit ThreadData(const std::function<void()>& newThreadFunc) : threadFunc(newThreadFunc) {}
};

#endif //UTILITYSTRUCTS_HPP
