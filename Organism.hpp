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

class Organism : public SimObject{
public:
    Organism(const uint64_t id,
        const int genomeSize,
        const SDL_FRect& boundingBox,
        const SimState& simState)
        : SimObject(id, boundingBox, simState),
          genome(Genome::createRandomGenome(genomeSize)),
          neuralNet(genome) {}

    Organism(const uint64_t id,
        const std::shared_ptr<SimState>& statePtr,
        const Organism& parent1,
        const Organism& parent2,
        const SimState& simState)
        : SimObject(id, parent1.getBoundingBox(), simState),
          genome(Genome::createGenomeFromParents(parent1.genome, parent2.genome)),
          neuralNet(genome) {}

    void mutateGenome();
    //[[nodiscard]] std::vector<std::pair<NeuronInputType, float>> getInputActivations() const {return neuralNet.getInputActivations();};
    void setInputActivations(const std::vector<std::pair<NeuronInputType, float>>& activations) {neuralNet.setInputActivations(activations);};
    //[[nodiscard]] std::vector<std::pair<NeuronOutputType, float>> getOutputActivations() const {return neuralNet.getOutputActivations();};

    static constexpr float acceleration = 3.0f;
    static constexpr float velocityMax = 10.0f;
    static constexpr float velocityDecay = 0.9f;

    [[nodiscard]] Vec2 getPosition() const {return {boundingBox.x, boundingBox.y};}
    [[nodiscard]] Vec2 getVelocity() const {return velocity;}
    void setVelocity(const Vec2& newVelocity) {
        if(abs(newVelocity.x) > velocityMax || abs(newVelocity.y) > velocityMax) {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Provided velocity for organism exceeds the maximum");
            return;
        }
        velocity = newVelocity;
    }

    void update(float deltaTime) override;
    void fixedUpdate() override;
    //void render(SDL_Renderer* renderer) const override;

private:
    Genome::Genome genome;
    NeuralNet neuralNet;
    Vec2 velocity = {0.0f, 0.0f};
    int hunger = 100;
    float timer = 0.0f;

    void updateInputs();
    void updateFromOutputs(float deltaTime);

    void move(const Vec2& moveVelocity);
    void tryEat(float activation);

    //void handleCollisions();
};


#endif //ORGANISM_HPP
