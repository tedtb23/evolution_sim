#ifndef ORGANISM_HPP
#define ORGANISM_HPP

#include "Genome.hpp"
#include "Traits.hpp"
#include "NeuralNet.hpp"
#include "SimObject.hpp"
#include "SimUtils.hpp"
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
        const SimUtils::SimState& simState,
        const bool inQuadTree)
        : SimObject(id, boundingBox, initialColor, simState, inQuadTree),
          genome(Genome::createRandomGenome(genomeSize)),
          traitGenome(Genome::createRandomTraitGenome()),
          neuralNet(genome) {initTraitValues();}

    Organism(const uint64_t id,
        const Organism& parent1,
        const Organism& parent2,
        const SDL_Color& initialColor,
        const SDL_FRect& boundingBox,
        const SimUtils::SimState& simState,
        const bool inQuadTree)
        : SimObject(id, boundingBox, initialColor, simState, inQuadTree),
          genome(Genome::createGenomeFromParents(parent1.genome, parent2.genome)),
          traitGenome(Genome::createTraitGenomeFromParents(parent1.traitGenome, parent2.traitGenome)),
          neuralNet(genome) {initTraitValues();}

    void mutateGenome();
    [[nodiscard]] std::vector<std::pair<NeuronInputType, float>> getInputActivations() const {return neuralNet.getInputActivations();}
    [[nodiscard]] std::vector<std::pair<NeuronOutputType, float>> getOutputActivations() const {return neuralNet.getOutputActivations();}
    [[nodiscard]] std::array<float, TRAITS_SIZE> getTraitValues() const {return traitValues;}

    static constexpr float velocityMax = 50.0f;
    static constexpr float velocityDecay = 0.9f;

    void addRaycastNeighbors(const std::vector<std::pair<uint64_t, Vec2>>& newRaycastNeighbors) {raycastNeighbors = newRaycastNeighbors;}
    void addNeighbors(const std::vector<std::pair<uint64_t, Vec2>>& newNeighbors) {neighbors = newNeighbors;}
    void addNeighbor(const std::pair<uint64_t, Vec2>& newNeighbor) {neighbors.emplace_back(newNeighbor);}
    void addCollisionID(const uint64_t collisionID) {collisionIDs.push_back(collisionID);}
    void clearCollisionIDs() {collisionIDs.clear();}
    [[nodiscard]] Vec2 getVelocity() const {return velocity;}
    void setVelocity(const Vec2& newVelocity) {
        if(abs(newVelocity.x) > velocityMax || abs(newVelocity.y) > velocityMax) {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Provided velocity for organism exceeds the maximum");
            return;
        }
        velocity = newVelocity;
    }
    void setDetectedDangerPheromone(const bool isDetected) {detectedDangerPheromone = isDetected;}
    [[nodiscard]] bool isEmittingDangerPheromone() const {return emitDangerPheromone;}
    [[nodiscard]] float getFertility() const {return traitValues[Traits::FERTILITY];}
    [[nodiscard]] uint8_t getTemperature() const {return temperature;}
    void setTemperature(const uint8_t newTemperature) {temperature = newTemperature;}
    [[nodiscard]] uint8_t getBreath() {return breath;}
    [[nodiscard]] float getOxygenSat() const {return oxygenSat;}
    void setOxygenSat(const float newOxygenSat) {oxygenSat = newOxygenSat;}
    [[nodiscard]] float getHydrogenSat() const {return hydrogenSat;}
    void setHydrogenSat(const float newHydrogenSat) {hydrogenSat = newHydrogenSat;}
    [[nodiscard]] uint8_t getAge() const {return age;}
    [[nodiscard]] uint8_t getHunger() const {return hunger;}
    [[nodiscard]] uint32_t getEnergy() const {return energy;}
    [[nodiscard]] bool shouldReproduce() const {return canReproduce;}
    void reproduce() {canReproduce = false; reproduced = true; energy = energy < 100 ? 0 : energy - 100;}
    void update(float deltaTime) override;
    void fixedUpdate() override;
    void render(SDL_Renderer* rendererPtr) const override;

private:
    Genome::TraitGenome traitGenome;
    std::array<float, TRAITS_SIZE> traitValues{};
    Genome::Genome genome;
    NeuralNet neuralNet;
    float acceleration = 10.0f;
    Vec2 velocity = {0.0f, 0.0f};
    uint8_t hunger = 100;
    uint8_t hungerStep = 0;
    uint32_t energy = 0;
    uint8_t age = 0;
    uint8_t temperature = 128;
    uint8_t breath = 100;
    float oxygenSat = 0.0f;
    float hydrogenSat = 0.0f;
    bool canReproduce = false;
    bool reproduced = false;
    bool detectedDangerPheromone = false;
    bool emitDangerPheromone = false;
    bool deleteSoon = false;
    float timer = 0.0f;
    float growthTimer = 0.0f;

    static constexpr Vec2 maxSize{25.0f, 25.0f};
    static constexpr uint8_t reproductionAge = 5;
    static constexpr float maxAcceleration = 80.0f;
    static constexpr uint8_t maxAge = 20;
    static constexpr uint8_t maxHungerStep = 50;
    static constexpr uint16_t growthEnergyThreshold = 300;
    static constexpr uint8_t inhaleStep = 30;
    static constexpr uint8_t exhaleStep = 10;

    std::vector<std::pair<uint64_t, Vec2>> neighbors;
    std::vector<std::pair<uint64_t, Vec2>> raycastNeighbors;
    std::vector<uint64_t> collisionIDs{};

    void initTraitValues();
    void grow();
    void updateHeatParams();
    void updateAtmosphereParams();

    void handleTimer(float deltaTime);
    void updateInputs();
    void updateFromOutputs(float deltaTime);


    void move(const Vec2& moveVelocity, const float deltaTime);
    template<typename SimObjectType> bool isColliding() const;
    template<typename SimObjectType> float findNearby(NeuronInputType neuronID, bool useRaycast = false);
    float checkBounds(NeuronInputType neuronID) const;
    void tryEat(float activation);
    std::array<SDL_Vertex, 3> getVelocityDirectionTriangleCoords() const;
    static float inverseActivation(float value, float rootPos, float strictness) {return (rootPos * 2.0f) / (1.0f + std::exp(value * strictness));}
};


#endif //ORGANISM_HPP
