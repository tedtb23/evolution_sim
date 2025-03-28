#include "Organism.hpp"
#include "Genome.hpp"
#include "NeuralNet.hpp"
#include "UtilityStructs.hpp"
#include "QuadTree.hpp"
#include "StaticSimObjects.hpp"
#include "SimObject.hpp"

void Organism::mutateGenome() {
    Genome::mutateGenome(&genome);
    neuralNet = NeuralNet(genome);
}

void Organism::update(const float deltaTime) {
    if(timer >= 1.00f) {
        hunger -= 10;
        if(hunger <= 0) {
            markedForDeletion = true;
        }
        timer = 0.0f;
    }else timer += deltaTime;

    Vec2 lastPosition = Vec2(boundingBox.x, boundingBox.y);
    updateInputs();
    updateFromOutputs(deltaTime);
    const auto qo = simState.quadTreePtr.get();
    if(Vec2(boundingBox.x, boundingBox.y) != lastPosition) {
        simState.quadTreePtr->remove(QuadTree::QuadTreeObject(id, SDL_FRect{lastPosition.x, lastPosition.y, boundingBox.w, boundingBox.h}));
        simState.quadTreePtr->insert(QuadTree::QuadTreeObject(id, boundingBox));
        color = SDL_Color{0, 0, 0, 255};
    }
    //handleCollisions();
}

void Organism::fixedUpdate() {
    velocity.x *= velocityDecay;
    velocity.y *= velocityDecay;
}

void Organism::updateInputs() {
    for(auto & [neuronID, activation] : neuralNet.getInputActivations()) {
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
    for(auto & [neuronID, activation] : neuralNet.getOutputActivations()) {
        if(activation < 0.50f) continue;
        switch(neuronID) {
            case MOVE_LEFT: {
                Vec2 moveVelocity = velocity;
                moveVelocity.x += -activation * acceleration * deltaTime;
                move(moveVelocity);
                break;
            }
            case MOVE_RIGHT: {
                Vec2 moveVelocity = velocity;
                moveVelocity.x += activation * acceleration * deltaTime;
                move(moveVelocity);
                break;
            }
            case MOVE_UP: {
                Vec2 moveVelocity = velocity;
                moveVelocity.y += -activation * acceleration * deltaTime;
                move(moveVelocity);
                break;
            }
            case MOVE_DOWN: {
                Vec2 moveVelocity = velocity;
                moveVelocity.y += activation * acceleration * deltaTime;
                move(moveVelocity);
                break;
            }
            case EAT: {
                tryEat(activation);
                break;
            }
            default: break;
        }
    }
}

void Organism::move(const Vec2& moveVelocity) {
    if(abs(moveVelocity.x) <= velocityMax && abs(moveVelocity.y) <= velocityMax) {
        velocity = moveVelocity;
        boundingBox.x += moveVelocity.x;
        boundingBox.y += moveVelocity.y;
    }
}

void Organism::tryEat(const float activation) {
    const int threshold = static_cast<int>(activation * 100.0f);
    std::vector<uint64_t> collidingIDs = simState.quadTreePtr->query(QuadTree::QuadTreeObject(id, SDL_FRect{boundingBox.x, boundingBox.y, boundingBox.w, boundingBox.h}));
    for(const uint64_t collidingID : collidingIDs) {
        std::shared_ptr<SimObject> objectPtr = (*simState.getFuncPtr)(collidingID);
        std::shared_ptr<Food> foodPtr = std::dynamic_pointer_cast<Food>(objectPtr);
        if(foodPtr && !foodPtr->shouldDelete() && hunger < threshold) {
            hunger += foodPtr->getNutritionalValue();
            if(hunger > 100) hunger = 100;
            foodPtr->markForDeletion();
        }
    }
}

//void Organism::handleCollisions() {

//}
