#ifndef ORGANISM_HPP
#define ORGANISM_HPP

#include "Genome.hpp"
#include "NeuralNet.hpp"
#include "SimObject.hpp"
#include "SimStructs.hpp"
#include "UtilityStructs.hpp"
#include "SDL3/SDL.h"
#include <vector>
#include <memory>
#include <utility>
#include <map>

class Organism : public SimObject{
public:
    Organism(const uint64_t id,
        const int genomeSize,
        const SDL_Color& initialColor,
        const SDL_FRect& boundingBox,
        const SimState& simState)
        : SimObject(id, boundingBox, initialColor, simState),
          genome(Genome::createRandomGenome(genomeSize)),
          neuralNet(genome) {}

    Organism(const uint64_t id,
        const Organism& parent1,
        const Organism& parent2,
        const SDL_Color& initialColor,
        const SDL_FRect& boundingBox,
        const SimState& simState)
        : SimObject(id, boundingBox, initialColor, simState),
          genome(Genome::createGenomeFromParents(parent1.genome, parent2.genome)),
          neuralNet(genome) {}

    void mutateGenome();
    [[nodiscard]] std::vector<std::pair<NeuronInputType, float>> getInputActivations() const {return neuralNet.getInputActivations();};
    [[nodiscard]] std::vector<std::pair<NeuronOutputType, float>> getOutputActivations() const {return neuralNet.getOutputActivations();};

    static constexpr float acceleration = 6.0f;
    static constexpr float velocityMax = 9.0f;
    static constexpr float velocityDecay = 0.9f;
    static constexpr int maxAge = 15;

    bool isEmittingDangerPheromone() {return emitDangerPheromone;}
    void addRaycastNeighbors(const std::vector<std::pair<uint64_t, Vec2>>& newRaycastNeighbors) {raycastNeighbors = newRaycastNeighbors;}
    void addNeighbors(const std::vector<std::pair<uint64_t, Vec2>>& newNeighbors) {neighbors = newNeighbors;}
    void clearRaycastNeighbors() {raycastNeighbors.clear();}
    void clearNeighbors() {neighbors.clear();}
    void addCollisionID(const uint64_t collisionID) {collisionIDs.emplace_back(collisionID);}
    void clearCollisionIDs() {collisionIDs.clear();}
    [[nodiscard]] Vec2 getPosition() const {return {boundingBox.x, boundingBox.y};}
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
    bool shouldReproduce() const {return canReproduce;}
    void reproduce() {canReproduce = false;}
    void update(float deltaTime) override;
    void fixedUpdate() override;
    void render(SDL_Renderer* renderer) const override;

private:
    Genome::Genome genome;
    NeuralNet neuralNet;
    Vec2 velocity = {0.0f, 0.0f};
    uint8_t hunger = 100;
    uint8_t age = 0;
    bool canReproduce = false;
    bool detectedDangerPheromone = false;
    bool emitDangerPheromone = false;
    float timer = 0.0f;

    std::vector<std::pair<uint64_t, Vec2>> neighbors;
    std::vector<std::pair<uint64_t, Vec2>> raycastNeighbors;
    std::vector<uint64_t> collisionIDs{};

    void handleTimer(float deltaTime);
    void updateInputs();
    void updateFromOutputs(float deltaTime);

    void move(const Vec2& moveVelocity);
    bool isOrganismColliding() const;
    bool isFoodColliding() const;
    float findNearbyOrganisms(NeuronInputType neuronID);
    float findNearbyFood(NeuronInputType neuronID) const;
    float findNearbyFire(NeuronInputType neuronID);
    void tryEat(float activation);
};


#endif //ORGANISM_HPP
