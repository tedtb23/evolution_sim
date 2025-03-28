#ifndef STATICSIMOBJECTS_HPP
#define STATICSIMOBJECTS_HPP

#include "SimObject.hpp"
#include "SimStructs.hpp"

class Simulation;

class Food : public SimObject {
public:
    Food(const uint64_t id, const SDL_FRect& boundingBox, const SimState& simState) : SimObject(id, boundingBox, simState) {}
    Food(const uint64_t id,
        const SDL_FRect& boundingBox,
        const SDL_Color& color,
        const int nutritionalValue,
        const SimState& simState) :
        SimObject(id, boundingBox, color, simState) {
        if(nutritionalValue <= 100 && nutritionalValue > 0)
            this->nutritionalValue = nutritionalValue;
    }

    [[nodiscard]] int getNutritionalValue() const{return nutritionalValue;}

    //void render(SDL_Renderer* renderer) const override;
private:
    int nutritionalValue = 100;
};

#endif //STATICSIMOBJECTS_HPP
