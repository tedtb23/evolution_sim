#ifndef ORGANISM_HPP
#define ORGANISM_HPP

#include "Genome.hpp"
#include "NeuralNet.hpp"
#include "SimState.hpp"
#include "SDL3/SDL_log.h"
#include <vector>
#include <utility>

struct Vec2 {
    float x;
    float y;
};

class Organism {
public:
    Organism(const int genomeSize, const Vec2 initialPosition)
        : genome(Genome::createRandomGenome(genomeSize)),
          neuralNet(genome),
          position(initialPosition) {};

    Organism(const Organism& parent1, const Organism& parent2)
            : genome(Genome::createGenomeFromParents(parent1.genome, parent2.genome)),
              neuralNet(genome),
              position(parent1.getPosition()){};

    void mutateGenome();

    [[nodiscard]] std::vector<std::pair<NeuronInputType, float>> getInputActivations() const {return neuralNet.getInputActivations();};
    void setInputActivations(const std::vector<std::pair<NeuronInputType, float>>& activations) {neuralNet.setInputActivations(activations);};
    [[nodiscard]] std::vector<std::pair<NeuronOutputType, float>> getOutputActivations() const {return neuralNet.getOutputActivations();};

    static constexpr float maxVelocity = 2.0f;
    static constexpr float width = 6.0f;
    static constexpr float height = 6.0f;

    Vec2 getPosition() const {return position;};
    Vec2 getVelocity() const {return velocity;};
    void setPosition(const Vec2& newPosition) {position = newPosition;};
    void setVelocity(const Vec2& newVelocity) {
        if(abs(newVelocity.x) > maxVelocity || abs(newVelocity.y) > maxVelocity) {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Provided velocity for organism exceeds the maximum");
            return;
        }
        velocity = newVelocity;
    };

    void update(const SimState& state, float deltaTime);

private:
    Genome::Genome genome;
    NeuralNet neuralNet;
    Vec2 position;
    Vec2 velocity = {0.0f, 0.0f};

    void updateInputs(const SimState& state);
    void updateFromOutputs(const SimState& state, float deltaTime);

    void move(const SimState& state, const Vec2& moveVelocity);
};


#endif //ORGANISM_HPP
