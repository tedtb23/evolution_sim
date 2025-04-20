#ifndef ORGANISM_HPP
#define ORGANISM_HPP

#include "Genome.hpp"
#include "Traits.hpp"
#include "NeuralNet.hpp"
#include "SimObject.hpp"
#include "SimStructs.hpp"
#include "UtilityStructs.hpp"
#include "SDL3/SDL.h"
#include <vector>
#include <memory>
#include <utility>
#include <map>
#include <array>

class Organism : public SimObject{
public:
    Organism(const uint64_t id,
        const uint16_t genomeSize,
        const SDL_Color& initialColor,
        const SDL_FRect& boundingBox,
        const SimState& simState)
        : SimObject(id, boundingBox, initialColor, simState),
          genome(Genome::createRandomGenome(genomeSize)),
          traitGenome(Genome::createRandomTraitGenome()),
          neuralNet(genome) {initTraitValues();}

    Organism(const uint64_t id,
        const Organism& parent1,
        const Organism& parent2,
        const SDL_Color& initialColor,
        const SDL_FRect& boundingBox,
        const SimState& simState)
        : SimObject(id, boundingBox, initialColor, simState),
          genome(Genome::createGenomeFromParents(parent1.genome, parent2.genome)),
          traitGenome(Genome::createTraitGenomeFromParents(parent1.traitGenome, parent2.traitGenome)),
          neuralNet(genome) {initTraitValues();}

    void mutateGenome();
    [[nodiscard]] std::vector<std::pair<NeuronInputType, float>> getInputActivations() const {return neuralNet.getInputActivations();}
    [[nodiscard]] std::vector<std::pair<NeuronOutputType, float>> getOutputActivations() const {return neuralNet.getOutputActivations();}
    [[nodiscard]] std::array<float, TRAITS_SIZE> getTraitValues() const {return traitValues;}

    static constexpr float velocityMax = 6.0f;
    static constexpr float velocityDecay = 0.9f;

    bool isEmittingDangerPheromone() const {return emitDangerPheromone;}
    void addRaycastNeighbors(const std::vector<std::pair<uint64_t, Vec2>>& newRaycastNeighbors) {raycastNeighbors = newRaycastNeighbors;}
    void addNeighbors(const std::vector<std::pair<uint64_t, Vec2>>& newNeighbors) {neighbors = newNeighbors;}
    //void clearRaycastNeighbors() {raycastNeighbors.clear();}
    //void clearNeighbors() {neighbors.clear();}
    void addCollisionID(const uint64_t collisionID) {collisionIDs.emplace_back(collisionID);}
    void clearCollisionIDs() {collisionIDs.clear();}
    [[nodiscard]] Vec2 getVelocity() const {return velocity;}
    void setVelocity(const Vec2& newVelocity) {
        if(abs(newVelocity.x) > velocityMax || abs(newVelocity.y) > velocityMax) {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Provided velocity for organism exceeds the maximum");
            return;
        }
        velocity = newVelocity;
    }

    uint8_t getAge() const {return age;}
    uint8_t getHunger() const {return hunger;}
    uint32_t getEnergy() const {return energy;}
    bool shouldReproduce() const {return canReproduce;}
    void reproduce() {canReproduce = false; reproduced = true; energy = energy < 100 ? 0 : energy - 100;}
    void update(float deltaTime) override;
    void fixedUpdate() override;
    void render(SDL_Renderer* renderer) const override;

private:
    Genome::TraitGenome traitGenome;
    std::array<float, TRAITS_SIZE> traitValues{};
    Genome::Genome genome;
    NeuralNet neuralNet;
    Vec2 velocity = {0.0f, 0.0f};
    uint8_t hunger = 100;
    uint32_t energy = 0;
    uint8_t age = 0;
    bool canReproduce = false;
    bool reproduced = false;
    bool detectedDangerPheromone = false;
    bool emitDangerPheromone = false;
    float timer = 0.0f;
    float growthTimer = 0.0f;

    static std::mt19937 mt;
    static constexpr Vec2 maxSize{16.0f, 16.0f};
    static constexpr float acceleration = 3.0f;
    static constexpr uint8_t reproductionAge = 5;
    static constexpr uint8_t maxAge = 20;
    static constexpr uint8_t hungerStep = 10;
    static constexpr uint16_t growthEnergyThreshold = 300;

    std::vector<std::pair<uint64_t, Vec2>> neighbors;
    std::vector<std::pair<uint64_t, Vec2>> raycastNeighbors;
    std::vector<uint64_t> collisionIDs{};

    void initTraitValues();
    void grow();

    void handleTimer(float deltaTime);
    void updateInputs();
    void updateFromOutputs(float deltaTime);

    void move(const Vec2& moveVelocity);
    template<typename SimObjectType> bool isColliding() const;
    template<typename SimObjectType> float findNearby(NeuronInputType neuronID, bool useRaycast = false);
    float checkBounds(NeuronInputType neuronID) const;
    void tryEat(float activation);
    std::array<SDL_Vertex, 3> getVelocityDirectionTriangleCoords() const;
    static float inverseActivation(float value, float strictness) {return 2.0f / (1.0f + std::exp(value * strictness));} //values approaching 0 result in values closer to 1.
};


#endif //ORGANISM_HPP
