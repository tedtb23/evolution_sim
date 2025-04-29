#ifndef STATICSIMOBJECTS_HPP
#define STATICSIMOBJECTS_HPP

#include "SimObject.hpp"
#include "SimUtils.hpp"
#include "SDL3/SDL.h"
#include "SDL3_image/SDL_image.h"
#include <filesystem>
#include <random>

class Food : public SimObject {
public:
    Food(const uint64_t id, const SDL_FRect& boundingBox, const SimUtils::SimState& simState, const bool inQuadTree) : SimObject(id, boundingBox, simState, inQuadTree) {}
    Food(const uint64_t id,
        const SDL_FRect& boundingBox,
        const SDL_Color& color,
        const int nutritionalValue,
        const SimUtils::SimState& simState,
        const bool inQuadTree) :
        SimObject(id, boundingBox, color, simState, inQuadTree) {
        if(nutritionalValue <= 100 && nutritionalValue > 0)
            this->nutritionalValue = nutritionalValue;
    }

    [[nodiscard]] int getNutritionalValue() const{return nutritionalValue;}

    void update(const float deltaTime) override {
        //handleTimers(deltaTime);
    }
    //void render(SDL_Renderer* rendererPtr) const override;
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
        const SimUtils::SimState& simState,
        const bool inQuadTree) :
        SimObject(id, boundingBox, {0, 0, 0, 0}, simState, inQuadTree),
        foodAmount(initialFoodAmount) {}

    [[nodiscard]] uint16_t getFoodAmount() const{return foodAmount;}
    void setFoodAmount(uint16_t newFoodAmount) {foodAmount = newFoodAmount;}
    uint16_t incrementFoodAmount() {return ++foodAmount;}
    uint16_t decrementFoodAmount() {
        if(foodAmount > 0) return --foodAmount;
        return 0;
    }

    void update(const float deltaTime) override {
        if(foodAmount == 0) markedForDeletion = true;
    }

    void render(SDL_Renderer* rendererPtr) const override {} //dont render

private:
    uint16_t foodAmount;
};

class Fire : public SimObject {
public:
    Fire(const uint64_t id, const SDL_FRect& boundingBox, const SimUtils::SimState& simState, SDL_Renderer* rendererPtr, const bool inQuadTree) :
        renderBoundingBox(boundingBox),
        SimObject(
            id,
            {
                boundingBox.x + 30,
                boundingBox.y + 40,
                static_cast<float>(boundingBox.w) - 60,
                static_cast<float>(boundingBox.h)
            },
            simState,
            inQuadTree) {
        initializeTexture(rendererPtr);
    }
    Fire(const uint64_t id,
         const SDL_FRect& boundingBox,
         const SDL_Color& color,
         const SimUtils::SimState& simState,
         SDL_Renderer* rendererPtr,
         const bool inQuadTree) :
        renderBoundingBox(boundingBox),
        SimObject(
            id,
            {
                boundingBox.x + 30,
                boundingBox.y + 40,
                static_cast<float>(boundingBox.w) - 60,
                static_cast<float>(boundingBox.h) - 40
            },
            simState,
            inQuadTree) {
        initializeTexture(rendererPtr);
    }
    ~Fire() override {
        SDL_DestroyTexture(texture);
        IMG_FreeAnimation(animation);
    }

    void update(const float deltaTime) override {
        handleTimers(deltaTime);
        SDL_Surface* writeableSurface;
        SDL_LockTextureToSurface(texture, NULL, &writeableSurface);
        SDL_memcpy(writeableSurface->pixels, animation->frames[framePos]->pixels, animation->frames[framePos]->h * animation->frames[framePos]->pitch);
        SDL_UnlockTexture(texture);
    }

    void render(SDL_Renderer* rendererPtr) const override {
        const SDL_FRect rect{0, 0, static_cast<float>(texture->w), static_cast<float>(texture->h)};
        SDL_RenderTexture(rendererPtr, texture, &rect, &renderBoundingBox);
    }

private:
    void initializeTexture(SDL_Renderer* rendererPtr) {
        const char* basePath = SDL_GetBasePath();
        std::filesystem::path base(basePath);
        std::filesystem::path imagesPath = base / "../resources/images/";
        std::filesystem::path firePath = imagesPath / "fire.gif";
        std::string firePathStr = firePath.lexically_normal().string();


        SDL_IOStream* stream = SDL_IOFromFile(firePathStr.c_str(), "r");
        if(!stream) SDL_Log("%s", SDL_GetError());
        animation = IMG_LoadGIFAnimation_IO(stream);
        if(!animation) SDL_Log("%s", SDL_GetError());
        if(!SDL_CloseIO(stream)) SDL_Log("%s", SDL_GetError());
        texture = SDL_CreateTexture(
            rendererPtr,
            (*animation->frames)->format,
            SDL_TEXTUREACCESS_STREAMING,
            animation->w,
            animation->h);
    }

    void handleTimers(const float deltaTime) {
        if(frameTimer >= static_cast<float>(animation->delays[framePos]) / 1000.0f) {
            framePos++;
            if(framePos >= animation->count) {
                framePos = 0;
            }
            frameTimer = 0.0f;
        }else frameTimer += deltaTime;
    }

    uint8_t framePos = 0;
    SDL_Texture* texture;
    IMG_Animation* animation;
    SDL_FRect renderBoundingBox;
    float frameTimer = 0.0f;
};

class Water : public SimObject {
public:
    Water(
        const uint64_t id,
        const SDL_FRect& boundingBox,
        const std::vector<SDL_Vertex>& vertexVec,
        const SimUtils::SimState& simState,
        const bool inQuadTree) :
            SimObject(id, boundingBox, {0, 0, 0, 0}, simState, inQuadTree),
            vertices(vertexVec) {}

private:
    std::vector<SDL_Vertex> vertices{};
    SDL_Texture* texture = nullptr;
};

class Pheromone : public SimObject {
public:
    Pheromone(const uint64_t id,
         const SDL_FRect& boundingBox,
         const SDL_Color& color,
         const SimUtils::SimState& simState,
        const bool inQuadTree) :
            SimObject(id, boundingBox, color, simState, inQuadTree), originalPosition(boundingBox.x, boundingBox.y) {}

    void update(const float deltaTime) override {
        handleTimers(deltaTime);
    }
    [[nodiscard]] Vec2 getPosition() const override {return {originalPosition.x, originalPosition.y};}
private:
    uint8_t age = 0;
    static constexpr uint8_t maxAge = 20;
    float ageTimer = 0.0f;
    Vec2 originalPosition;

    void handleTimers(const float deltaTime) {
        if(ageTimer >= 1.0f) {
            std::bernoulli_distribution distDriftChance(0.05);
            if(distDriftChance(SimUtils::mt)) {
                std::bernoulli_distribution distDriftDirectionX(0.50);
                std::bernoulli_distribution distDriftDirectionY(0.50);
                if(distDriftDirectionX(SimUtils::mt)) boundingBox.x += 2.0f;
                else boundingBox.x -= 2.0f;
                if(distDriftDirectionY(SimUtils::mt)) boundingBox.y += 2.0f;
                else boundingBox.y -= 2.0f;
            }
            age++;
        if(age >= maxAge) {
            markedForDeletion = true;
        }
        ageTimer = 0.0f;
        }else ageTimer += deltaTime;
    }
};

#endif //STATICSIMOBJECTS_HPP
