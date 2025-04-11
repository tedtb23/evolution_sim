#ifndef UTILITYSTRUCTS_HPP
#define UTILITYSTRUCTS_HPP

#include <cmath>

struct Vec2 {
    float x;
    float y;

    constexpr Vec2(const float x, const float y) : x(x), y(y) {};

    [[nodiscard]] float getDistanceToOrigin() const {
        return static_cast<float>(sqrt((x*x) + (y*y)));
    }

    bool operator==(const Vec2& other) const = default;
    bool operator<(const Vec2& other) const{
        return getDistanceToOrigin() < other.getDistanceToOrigin();
    }
    bool operator>(const Vec2& other) const{
        return getDistanceToOrigin() > other.getDistanceToOrigin();
    }
};

#endif //UTILITYSTRUCTS_HPP
