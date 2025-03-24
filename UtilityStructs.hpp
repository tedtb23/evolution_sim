#ifndef UTILITYSTRUCTS_HPP
#define UTILITYSTRUCTS_HPP

struct Vec2 {
    float x;
    float y;

    Vec2(float x, float y) : x(x), y(y) {};

    bool operator==(const Vec2& other) const{
        return this->x == other.x && this->y == other.y;
    }
};

#endif //UTILITYSTRUCTS_HPP
