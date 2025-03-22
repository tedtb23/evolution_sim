#include "Organism.hpp"
#include "Genome.hpp"
#include "NeuralNet.hpp"
#include "UtilityStructs.hpp"

void Organism::mutateGenome() {
    Genome::mutateGenome(&genome);
    neuralNet = NeuralNet(genome);
}

void Organism::update(const float deltaTime) {
    simStatePtr->quadTree.insert(SimObject{id, SDL_FRect{position.x, position.y, width, height}});
    updateInputs();
    updateFromOutputs(deltaTime);
    handleCollisions();
}

void Organism::fixedUpdate() {
    velocity.x *= velocityDecay;
    velocity.y *= velocityDecay;
}

void Organism::setColor() {
    
}

void Organism::updateInputs() {
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

void Organism::updateFromOutputs(const float deltaTime) {
    for(auto & [neuronID, activation] : getOutputActivations()) {
        if(activation < 0.50f) continue;
        switch(neuronID) {
            case MOVE_LEFT: {
                Vec2 moveVelocity = velocity;
                //moveVelocity.x += -activation * deltaTime;
                moveVelocity.x += -acceleration * deltaTime;
                move(moveVelocity);
                break;
            }
            case MOVE_RIGHT: {
                Vec2 moveVelocity = velocity;
                //moveVelocity.x += activation * deltaTime;
                moveVelocity.x += acceleration * deltaTime;
                move(moveVelocity);
                break;
            }
            case MOVE_UP: {
                Vec2 moveVelocity = velocity;
                //moveVelocity.y += -activation * deltaTime;
                moveVelocity.y += -acceleration * deltaTime;
                move(moveVelocity);
                break;
            }
            case MOVE_DOWN: {
                Vec2 moveVelocity = velocity;
                //moveVelocity.y += activation * deltaTime;
                moveVelocity.y += acceleration * deltaTime;
                move(moveVelocity);
                break;
            }
            default: break;
        }
    }
}

void Organism::move(const Vec2& moveVelocity) {
    if(abs(moveVelocity.x) <= velocityMax && abs(moveVelocity.y) <= velocityMax) {
        velocity = moveVelocity;
        position.x += moveVelocity.x;
        position.y += moveVelocity.y;
    }
}

void Organism::handleCollisions() {
    const auto leftBound = static_cast<float>(simStatePtr->simBounds.x);
    const auto rightBound = static_cast<float>(simStatePtr->simBounds.x + simStatePtr->simBounds.w);
    const auto topBound = static_cast<float>(simStatePtr->simBounds.y);
    const auto bottomBound = static_cast<float>(simStatePtr->simBounds.y + simStatePtr->simBounds.h);

    if(position.x < leftBound) position.x = leftBound;
    if(position.x + width > rightBound) position.x = rightBound - width;
    if(position.y < topBound) position.y = topBound;
    if(position.y + height > bottomBound) position.y = bottomBound - height;
}
