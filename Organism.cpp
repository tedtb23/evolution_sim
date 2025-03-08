#include "Organism.hpp"
#include "Genome.hpp"
#include "NeuralNet.hpp"
#include "SimState.hpp"

void Organism::mutateGenome() {
    Genome::mutateGenome(&genome);
    neuralNet = NeuralNet(genome);
}

void Organism::update(const SimState& state, const float deltaTime) {
    updateInputs(state);
    updateFromOutputs(state, deltaTime);
}

void Organism::updateInputs(const SimState& state) {
    for(auto & [neuronID, activation] : getInputActivations()) {
        activation = 1.0f; //temporary
        switch(neuronID) {
            case SIGHT_OBJECT_FORWARD: {
                //todo ...
            }
            default: break;
        }
    }
}

void Organism::updateFromOutputs(const SimState& state, const float deltaTime) {
    for(auto & [neuronID, activation] : getOutputActivations()) {
        switch(neuronID) {
            case MOVE_LEFT: {
                Vec2 moveVelocity = velocity;
                moveVelocity.x += -activation * deltaTime;
                //moveVelocity.x += -0.0001f;
                move(state, moveVelocity);
            }
            case MOVE_RIGHT: {
                Vec2 moveVelocity = velocity;
                moveVelocity.x += activation * deltaTime;
                //moveVelocity.x += 0.0001f;
                move(state, moveVelocity);
            }
            case MOVE_UP: {
                Vec2 moveVelocity = velocity;
                moveVelocity.y += -activation * deltaTime;
                //moveVelocity.y += -0.0001f;
                move(state, moveVelocity);
            }
            case MOVE_DOWN: {
                Vec2 moveVelocity = velocity;
                moveVelocity.y += activation * deltaTime;
                //moveVelocity.y += 0.0001f;
                move(state, moveVelocity);
            }
            default: break;
        }
    }
}

void Organism::move(const SimState& state, const Vec2& moveVelocity) {
    int windowWidth, windowHeight;
    SDL_GetWindowSize(state.window, &windowWidth, &windowHeight);

    if(abs(moveVelocity.x) <= Organism::maxVelocity && abs(moveVelocity.y) <= Organism::maxVelocity) velocity = moveVelocity;

    Vec2 movePosition = position;
    movePosition.x += moveVelocity.x;
    movePosition.y += moveVelocity.y;

    if(movePosition.x > 0.0f && movePosition.x + width < static_cast<float>(windowWidth) &&
    movePosition.y > 0.0f && movePosition.y + height < static_cast<float>(windowHeight)) position = movePosition;
}
