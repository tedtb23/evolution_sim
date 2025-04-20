#ifndef STATICSIMOBJECTS_HPP
#define STATICSIMOBJECTS_HPP

#include "SimObject.hpp"
#include "SimStructs.hpp"

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
        //handleTimers(deltaTime);
    }
    //void render(SDL_Renderer* renderer) const override;
private:
    int nutritionalValue = 100;
    //uint8_t age = 0;
    //static constexpr uint8_t maxAge = 20;
    //float ageTimer = 0.0f;

    //void handleTimers(const float deltaTime) {
        //if(ageTimer >= 1.0f) {
        //    age++;
        //    if(age >= maxAge) {
        //        markedForDeletion = true;
        //    }
        //    ageTimer = 0.0f;
        //}else ageTimer += deltaTime;
    //}
};

class FoodSpawnRange : public SimObject {
public:
    FoodSpawnRange(
            const uint64_t id,
            const SDL_FRect& boundingBox,
            const uint16_t initialFoodAmount,
            const SimState& simState) :
            SimObject(id, boundingBox, {0, 0, 0, 0}, simState),
            foodAmount(initialFoodAmount) {}

    [[nodiscard]] uint16_t getFoodAmount() const{return foodAmount;}
    uint16_t decreaseFoodAmount() {
        if(foodAmount > 0) return --foodAmount;
        return 0;
    }

    void update(const float deltaTime) override {
        if(foodAmount == 0) markedForDeletion = true;
    }

    void render(SDL_Renderer* renderer) const override {}

private:
    uint16_t foodAmount;
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

class Water : public SimObject {
public:
    Water(
        const uint64_t id,
        const SDL_FRect& boundingBox,
        const std::vector<SDL_Vertex>& vertexVec,
        const SimState& simState) :
            SimObject(id, boundingBox, {0, 0, 0, 0}, simState)
            //vertices(vertexVec) {
    {
        for(int i = 0; i < 10; i++) {
            vertices.push_back(SDL_Vertex{{static_cast<float>((i * 10)), static_cast<float>((i * 10))}, {0, 0, 1.0, 1.0}});
        }
    }

    void update(const float deltaTime) override {

    }

    void render(SDL_Renderer* renderer) const override {
        SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
        SDL_Vertex vert[vertices.size()];
        for(int i = 0; i < vertices.size(); i++) {
            vert[i] = vertices[i];
        }

        if(!SDL_RenderGeometry(renderer, NULL, vertices.data(), vertices.size(), NULL, 0)) SDL_Log("%s", SDL_GetError());
    }

private:
    std::vector<SDL_Vertex> vertices{};
    SDL_Texture* texture = nullptr;
};

#endif //STATICSIMOBJECTS_HPP
