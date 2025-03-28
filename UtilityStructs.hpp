#ifndef UTILITYSTRUCTS_HPP
#define UTILITYSTRUCTS_HPP

struct Vec2 {
    float x;
    float y;

    Vec2(const float x, const float y) : x(x), y(y) {};

    bool operator==(const Vec2& other) const = default;
};

#endif //UTILITYSTRUCTS_HPP
