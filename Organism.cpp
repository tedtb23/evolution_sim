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
    color = SDL_Color{0, 0, 0, 255};

    handleTimer(deltaTime);

    Vec2 lastPosition = Vec2(boundingBox.x, boundingBox.y);

    updateInputs();
    updateFromOutputs(deltaTime);
    if(Vec2(boundingBox.x, boundingBox.y) != lastPosition) {
        simState.quadTreePtr->remove(QuadTree::QuadTreeObject(id, SDL_FRect{lastPosition.x, lastPosition.y, boundingBox.w, boundingBox.h}));
        simState.quadTreePtr->insert(QuadTree::QuadTreeObject(id, boundingBox));
    }
    collisionIDs.clear();
    //neighborIDs.clear();
}

void Organism::handleTimer(const float deltaTime) {
    if(timer >= 1.00f) {
        hunger -= 1;
        if(hunger <= 0) {
            markedForDeletion = true;
        }
        timer = 0.0f;
    }else timer += deltaTime;
}

void Organism::fixedUpdate() {
    neighborIDs = simState.quadTreePtr->getNearestNeighbors(QuadTree::QuadTreeObject(id, boundingBox));
    velocity.x *= velocityDecay;
    velocity.y *= velocityDecay;
}

void Organism::updateInputs() {
    std::vector<std::pair<NeuronInputType, float>> activations = neuralNet.getInputActivations();
    for(auto & [neuronID, activation] : activations) {
        switch(neuronID) {
            case HUNGER: {
                activation = ((-0.01f) * static_cast<float>(hunger)) + 1;
                break;
            }
            case FOOD_LEFT:
            case FOOD_RIGHT:
            case FOOD_UP:
            case FOOD_DOWN:
            case FOOD_COLLISION:
                activation = findNearbyFood(neuronID);
                break;

            default:
                activation = 1.00f;
                break;
        }
    }
    neuralNet.setInputActivations(activations);
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

float Organism::findNearbyFood(NeuronInputType neuronID) const{
    if(neighborIDs.empty()) return 0.0f;

    std::function<float (const SDL_FRect& rect, const SDL_FRect& range)> distanceLambda;
    float minDistance = INFINITY;

    switch(neuronID) {
        case FOOD_LEFT:
            distanceLambda = [] (const SDL_FRect& rect, const SDL_FRect& range) {return rect.x - (range.x + range.w);};
            break;
        case FOOD_RIGHT:
            distanceLambda = [] (const SDL_FRect& rect, const SDL_FRect& range) {return range.x - (rect.x + rect.w);};
            break;
        case FOOD_UP:
            distanceLambda = [] (const SDL_FRect& rect, const SDL_FRect& range) {return rect.y - (range.y + range.h);};
            break;
        case FOOD_DOWN:
            distanceLambda = [] (const SDL_FRect& rect, const SDL_FRect& range) {return range.y - (rect.y + rect.h);};
            break;
        case FOOD_COLLISION: {
            for(const auto collisionID : collisionIDs) {
                std::shared_ptr<SimObject> objectPtr = (*simState.getFuncPtr)(collisionID);
                std::shared_ptr<Food> foodPtr = std::dynamic_pointer_cast<Food>(objectPtr);
                if(!foodPtr) continue;
                return 1.0f;
            }
            return 0.0f;
        }
        default: return 0.0f;
    }
    for(const auto neighborID : neighborIDs) {
        std::shared_ptr<SimObject> objectPtr = (*simState.getFuncPtr)(neighborID);
        std::shared_ptr<Food> foodPtr = std::dynamic_pointer_cast<Food>(objectPtr);
        if(foodPtr) {
            const SDL_FRect& foodBoundingBox = foodPtr->getBoundingBox();
            float distance = distanceLambda(boundingBox, foodBoundingBox);
            if(distance < 0.0f) distance = INFINITY;
            minDistance = std::min(distance, minDistance);
        }
    }
    if(minDistance == INFINITY) return 0.0f;
    const float result = 2.0f / (1.0f + std::exp(minDistance));
    return result;
}

void Organism::tryEat(const float activation) {
    const int threshold = static_cast<int>(activation * 100.0f);
    for (const uint64_t collisionID: collisionIDs) {
        std::shared_ptr<SimObject> objectPtr = (*simState.getFuncPtr)(collisionID);
        std::shared_ptr<Food> foodPtr = std::dynamic_pointer_cast<Food>(objectPtr);
        if (foodPtr && !foodPtr->shouldDelete() && hunger < threshold) {
            hunger += foodPtr->getNutritionalValue();
            if (hunger > 100) hunger = 100;
            foodPtr->markForDeletion();
        }
    }
}
