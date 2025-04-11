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

    void update(const float deltaTime) override {
        handleTimers(deltaTime);
    }
    //void render(SDL_Renderer* renderer) const override;
private:
    int nutritionalValue = 100;
    uint8_t age = 0;
    static constexpr uint8_t maxAge = 20;
    float ageTimer = 0.0f;

    void handleTimers(const float deltaTime) {
        if(ageTimer >= 1.0f) {
            age++;
            if(age >= maxAge) {
                markedForDeletion = true;
            }
            ageTimer = 0.0f;
        }else ageTimer += deltaTime;
    }
};

class Fire : public SimObject {
public:
    Fire(const uint64_t id, const SDL_FRect& boundingBox, const SimState& simState) : SimObject(id, boundingBox, simState) {}
    Fire(const uint64_t id,
         const SDL_FRect& boundingBox,
         const SDL_Color& color,
         const SimState& simState) :
            SimObject(id, boundingBox, color, simState) {}
private:

};

#endif //STATICSIMOBJECTS_HPP
