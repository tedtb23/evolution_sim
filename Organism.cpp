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
    //color = SDL_Color{0, 0, 0, 255};

    handleTimer(deltaTime);

    Vec2 lastPosition = Vec2(boundingBox.x, boundingBox.y);

    updateInputs();
    updateFromOutputs(deltaTime);
    if (Vec2(boundingBox.x, boundingBox.y) != lastPosition) {
        simState.quadTreePtr->remove(
                QuadTree::QuadTreeObject(id, SDL_FRect{lastPosition.x, lastPosition.y, boundingBox.w, boundingBox.h}));
        simState.quadTreePtr->insert(QuadTree::QuadTreeObject(id, boundingBox));
    }
}

void Organism::handleTimer(const float deltaTime) {
    if(timer >= 1.00f) {
        hunger -= 10;
        age++;

        if(hunger <= 0 || age >= maxAge) markedForDeletion = true;

        timer = 0.0f;
    }else timer += deltaTime;
}

void Organism::fixedUpdate() {
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
                activation = findNearbyFood(neuronID);
                break;
            case FOOD_COLLISION:
                activation = isFoodColliding() ? 1.0f : 0.0f;
                break;
            case ORGANISM_LEFT:
            case ORGANISM_RIGHT:
            case ORGANISM_UP:
            case ORGANISM_DOWN:
                activation = findNearbyOrganisms(neuronID);
                break;
            case ORGANISM_COLLISION:
                activation = isOrganismColliding() ? 1.0 : 0.0f;
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

bool Organism::isOrganismColliding() const {
    for(const auto collisionID : collisionIDs) {
        std::shared_ptr<SimObject> objectPtr = (*simState.getFuncPtr)(collisionID);
        std::shared_ptr<Organism> organismPtr = std::dynamic_pointer_cast<Organism>(objectPtr);
        if(!organismPtr) continue;
        return true;
    }
    return false;
}

bool Organism::isFoodColliding() const {
    for(const auto collisionID : collisionIDs) {
        std::shared_ptr<SimObject> objectPtr = (*simState.getFuncPtr)(collisionID);
        std::shared_ptr<Food> foodPtr = std::dynamic_pointer_cast<Food>(objectPtr);
        if(!foodPtr) continue;
        return true;
    }
    return false;
}

float Organism::findNearbyOrganisms(NeuronInputType neuronID) const {
    if(neighbors.empty()) return 0.0f;

    float distance = NAN;

    for(const auto& [neighborID, neighborDistance] : neighbors) {
        std::shared_ptr<SimObject> objectPtr = (*simState.getFuncPtr)(neighborID);
        std::shared_ptr<Organism> organismPtr = std::dynamic_pointer_cast<Organism>(objectPtr);
        if(organismPtr) {
            switch(neuronID) {
                case ORGANISM_LEFT:
                    if(neighborDistance.x > 0.0f) continue;
                    distance = -neighborDistance.x;
                    break;
                case ORGANISM_RIGHT:
                    if(neighborDistance.x < 0.0f) continue;
                    distance = neighborDistance.x;
                    break;
                case ORGANISM_UP:
                    if(neighborDistance.y > 0.0f) continue;
                    distance = -neighborDistance.y;
                    break;
                case ORGANISM_DOWN:
                    if(neighborDistance.y < 0.0f) continue;
                    distance = neighborDistance.y;
                    break;
                default: return 0.0f;
            }
            break; //if we make it here the distance should always be set
        }
    }
    if(std::isnan(distance)) {
        return 0.0f;
    }
    return 2.0f / (1.0f + std::exp(distance)); //distances approaching 0 result in values closer to 1.
}

float Organism::findNearbyFood(NeuronInputType neuronID) const{
    if(neighbors.empty()) return 0.0f;

    float distance = NAN;

    for(const auto& [neighborID, neighborDistance] : neighbors) {
        std::shared_ptr<SimObject> objectPtr = (*simState.getFuncPtr)(neighborID);
        std::shared_ptr<Food> foodPtr = std::dynamic_pointer_cast<Food>(objectPtr);
        if(foodPtr) {
            switch(neuronID) {
                case FOOD_LEFT:
                    if(neighborDistance.x > 0.0f) continue;
                    distance = -neighborDistance.x;
                    break;
                case FOOD_RIGHT:
                    if(neighborDistance.x < 0.0f) continue;
                    distance = neighborDistance.x;
                    break;
                case FOOD_UP:
                    if(neighborDistance.y > 0.0f) continue;
                    distance = -neighborDistance.y;
                    break;
                case FOOD_DOWN:
                    if(neighborDistance.y < 0.0f) continue;
                    distance = neighborDistance.y;
                    break;
                default: return 0.0f;
            }
            break; //if we make it here the distance should always be set
        }
    }
    if(std::isnan(distance)) {
        return 0.0f;
    }
    return 2.0f / (1.0f + std::exp(distance)); //distances approaching 0 result in values closer to 1.
}

void Organism::tryEat(const float activation) {
    const int threshold = static_cast<int>(activation * 100.0f);
    for (const uint64_t collisionID: collisionIDs) {
        std::shared_ptr<SimObject> objectPtr = (*simState.getFuncPtr)(collisionID);
        std::shared_ptr<Food> foodPtr = std::dynamic_pointer_cast<Food>(objectPtr);
        if (foodPtr && !foodPtr->shouldDelete() && hunger < threshold) {
            hunger += foodPtr->getNutritionalValue();
            canReproduce = true;
            if (hunger > 100) hunger = 100;
            foodPtr->markForDeletion();
        }
    }
}
