#ifndef ORGANISM_HPP
#define ORGANISM_HPP

#include "Genome.hpp"
#include "NeuralNet.hpp"
#include "SimStructs.hpp"
#include "SDL3/SDL.h"
#include <vector>
#include <memory>
#include <utility>

class Organism {
public:
    Organism(const uint64_t id, const std::shared_ptr<SimState>& statePtr, const int genomeSize, const Vec2& initialPosition)
        : id(id),
          simStatePtr(statePtr),
          genome(Genome::createRandomGenome(genomeSize)),
          neuralNet(genome),
          position(initialPosition) {};

    Organism(const uint64_t id, const std::shared_ptr<SimState>& statePtr, const Organism& parent1, const Organism& parent2)
        : id(id),
          simStatePtr(statePtr),
          genome(Genome::createGenomeFromParents(parent1.genome, parent2.genome)),
          neuralNet(genome),
          position(parent1.getPosition()) {};

    void mutateGenome();
    [[nodiscard]] std::vector<std::pair<NeuronInputType, float>> getInputActivations() const {return neuralNet.getInputActivations();};
    void setInputActivations(const std::vector<std::pair<NeuronInputType, float>>& activations) {neuralNet.setInputActivations(activations);};
    [[nodiscard]] std::vector<std::pair<NeuronOutputType, float>> getOutputActivations() const {return neuralNet.getOutputActivations();};

    static constexpr float acceleration = 3.0f;
    static constexpr float velocityMax = 10.0f;
    static constexpr float velocityDecay = 0.9f;
    static constexpr float width = 6.0f;
    static constexpr float height = 6.0f;

    [[nodiscard]] Vec2 getPosition() const {return position;};
    [[nodiscard]] Vec2 getVelocity() const {return velocity;};
    void setPosition(const Vec2& newPosition) {position = newPosition;};
    void setVelocity(const Vec2& newVelocity) {
        if(abs(newVelocity.x) > velocityMax || abs(newVelocity.y) > velocityMax) {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Provided velocity for organism exceeds the maximum");
            return;
        }
        velocity = newVelocity;
    };
    SDL_Color getColor() const {return color;};

    void update(float deltaTime);
    void fixedUpdate();

private:
    uint64_t id;
    std::shared_ptr<SimState> simStatePtr;
    Genome::Genome genome;
    NeuralNet neuralNet;
    Vec2 position;
    Vec2 velocity = {0.0f, 0.0f};
    SDL_Color color{};

    void setColor();

    void updateInputs();
    void updateFromOutputs(float deltaTime);

    void move(const Vec2& moveVelocity);

    void handleCollisions();
};


#endif //ORGANISM_HPP
