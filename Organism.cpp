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
    color = {255, 85, 0, 255};
    boundingBox.w = 10.0f;
    boundingBox.h = 10.0f;
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

void Organism::render(SDL_Renderer* renderer) const {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    const SDL_FRect organismRect = {boundingBox.x, boundingBox.y, boundingBox.w, boundingBox.h};
    SDL_RenderFillRect(renderer, &organismRect);

    if(emitDangerPheromone) {
        static std::mt19937 mt{std::random_device{}()};
        std::uniform_int_distribution<int> distX(boundingBox.x - 10, boundingBox.x + 10);
        std::uniform_int_distribution<int> distY(boundingBox.y - 10, boundingBox.y + 10);

        SDL_SetRenderDrawColor(renderer, color.r - 100, color.g - 50, color.b + 100, color.a);
        for(int i = 0; i < 10; i++) {
            const SDL_FRect pheromoneRect = {static_cast<float>(distX(mt)), static_cast<float>(distY(mt)), 2, 2};
            SDL_RenderFillRect(renderer, &pheromoneRect);
        }
    }
}

void Organism::handleTimer(const float deltaTime) {
    if(timer >= 1.00f) {
        hunger -= 10;
        age++;

        if(age == maxAge) canReproduce = true;
        if(hunger <= 0 || (age >= maxAge && !canReproduce)) {
            markedForDeletion = true;
            boundingBox.w = 12.0f;
            boundingBox.h = 12.0f;
            color = {255, 0, 0, 255};
        }

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
            case FIRE_LEFT:
            case FIRE_RIGHT:
            case FIRE_UP:
            case FIRE_DOWN:
                activation = findNearbyFire(neuronID);
                break;
            case DETECT_DANGER_PHEROMONE: {
                if(detectedDangerPheromone) {
                    activation = 1.0f;
                    detectedDangerPheromone = false;
                }else {
                    activation = 0.0f;
                }
                break;
            }

            default:
                activation = 1.00f;
                break;
        }
    }
    neuralNet.setInputActivations(activations);
}

void Organism::updateFromOutputs(const float deltaTime) {
    for(auto & [neuronID, activation] : neuralNet.getOutputActivations()) {
        //if(activation < 0.50f) continue;
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
            //case EMIT_DANGER_PHEROMONE: {
            //   if(activation >= 0.5f) {
            //        emitDangerPheromone = true;
            //    }else {
            //        emitDangerPheromone = false;
            //    }
            //    break;
            //}
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

float Organism::findNearbyOrganisms(NeuronInputType neuronID) {
    bool usingRaycast = false;
    const std::vector<std::pair<uint64_t, Vec2>>* searchObjectsPtr;
    if(!raycastNeighbors.empty()) {
        searchObjectsPtr = &raycastNeighbors;
        usingRaycast = true;
    }else if(!neighbors.empty()){
        searchObjectsPtr = &neighbors;
    }else return 0.0f;

    float distance = NAN;

    for(const auto& [neighborID, neighborDistance] : *searchObjectsPtr) {
        std::shared_ptr<SimObject> objectPtr = (*simState.getFuncPtr)(neighborID);
        std::shared_ptr<Organism> organismPtr = std::dynamic_pointer_cast<Organism>(objectPtr);
        if(organismPtr) {
            if(organismPtr->isEmittingDangerPheromone()) {detectedDangerPheromone = true;}
            switch(neuronID) {
                case ORGANISM_LEFT:
                    if(neighborDistance.x >= 0.0f) continue;
                    distance = -neighborDistance.x;
                    break;
                case ORGANISM_RIGHT:
                    if(neighborDistance.x <= 0.0f) continue;
                    distance = neighborDistance.x;
                    break;
                case ORGANISM_UP:
                    if(neighborDistance.y >= 0.0f) continue;
                    distance = -neighborDistance.y;
                    break;
                case ORGANISM_DOWN:
                    if(neighborDistance.y <= 0.0f) continue;
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

    //distances approaching 0 result in values closer to 1.
    if(usingRaycast) {
        return 2.0f / (1.0f + std::exp(distance * 0.0025f));
    }else {
        return 2.0f / (1.0f + std::exp(distance * 0.05f));
    }
}

float Organism::findNearbyFood(NeuronInputType neuronID) const{
    bool usingRaycast = false;
    const std::vector<std::pair<uint64_t, Vec2>>* searchObjectsPtr;
    if(!raycastNeighbors.empty()) {
        searchObjectsPtr = &raycastNeighbors;
        usingRaycast = true;
    }else if(!neighbors.empty()){
        searchObjectsPtr = &neighbors;
    }else return 0.0f;

    float distance = NAN;

    for(const auto& [neighborID, neighborDistance] : *searchObjectsPtr) {
        std::shared_ptr<SimObject> objectPtr = (*simState.getFuncPtr)(neighborID);
        std::shared_ptr<Food> foodPtr = std::dynamic_pointer_cast<Food>(objectPtr);
        if(foodPtr) {
            switch(neuronID) {
                case FOOD_LEFT:
                    if(neighborDistance.x >= 0.0f) continue;
                    distance = -neighborDistance.x;
                    break;
                case FOOD_RIGHT:
                    if(neighborDistance.x <= 0.0f) continue;
                    distance = neighborDistance.x;
                    break;
                case FOOD_UP:
                    if(neighborDistance.y >= 0.0f) continue;
                    distance = -neighborDistance.y;
                    break;
                case FOOD_DOWN:
                    if(neighborDistance.y <= 0.0f) continue;
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
    //distances approaching 0 result in values closer to 1.
    if(usingRaycast) {
        return 2.0f / (1.0f + std::exp(distance * 0.0025f));
    }else {
        return 2.0f / (1.0f + std::exp(distance * 0.05f));
    }
}

float Organism::findNearbyFire(NeuronInputType neuronID) {
    bool usingRaycast = false;
    const std::vector<std::pair<uint64_t, Vec2>>* searchObjectsPtr;
    if(!raycastNeighbors.empty()) {
        searchObjectsPtr = &raycastNeighbors;
        usingRaycast = true;
    }else if(!neighbors.empty()){
        searchObjectsPtr = &neighbors;
    }else return 0.0f;

    float distance = NAN;

    for(const auto& [neighborID, neighborDistance] : *searchObjectsPtr) {
        std::shared_ptr<SimObject> objectPtr = (*simState.getFuncPtr)(neighborID);
        std::shared_ptr<Fire> firePtr = std::dynamic_pointer_cast<Fire>(objectPtr);
        if(firePtr) {
            switch(neuronID) {
                case FIRE_LEFT:
                    if(neighborDistance.x >= 0.0f) continue;
                    distance = -neighborDistance.x;
                    break;
                case FIRE_RIGHT:
                    if(neighborDistance.x <= 0.0f) continue;
                    distance = neighborDistance.x;
                    break;
                case FIRE_UP:
                    if(neighborDistance.y >= 0.0f) continue;
                    distance = -neighborDistance.y;
                    break;
                case FIRE_DOWN:
                    if(neighborDistance.y <= 0.0f) continue;
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

    if(distance <= 0.5f) {
        emitDangerPheromone = true;
    }else {
        emitDangerPheromone = false;
    }

    //distances approaching 0 result in values closer to 1.
    if(usingRaycast) {
        return 2.0f / (1.0f + std::exp(distance * 0.0025f));
    }else {
        return 2.0f / (1.0f + std::exp(distance * 0.05f));
    }
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
