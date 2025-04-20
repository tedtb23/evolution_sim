#include "Organism.hpp"
#include "Genome.hpp"
#include "Traits.hpp"
#include "NeuralNet.hpp"
#include "UtilityStructs.hpp"
#include "QuadTree.hpp"
#include "StaticSimObjects.hpp"
#include "SimObject.hpp"
#include <array>

std::mt19937 Organism::mt{std::random_device{}()};

void Organism::mutateGenome() {
    Genome::mutateGenome(&genome);
    neuralNet = NeuralNet(genome);
    Genome::mutateTraitGenome(&traitGenome);
    initTraitValues();
    color = {255, 85, 0, 255};
    boundingBox.w = 10.0f;
    boundingBox.h = 10.0f;
}

void Organism::initTraitValues() {
    for(int i = 0; i < traitValues.size(); i++) {
        traitValues[i] = static_cast<float>(traitGenome.traits[i]) / static_cast<float>(UINT16_MAX);
    }
}

void Organism::update(const float deltaTime) {
    if(emitDangerPheromone) emitDangerPheromone = false;

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

void Organism::grow() {
    if(energy < growthEnergyThreshold) return;

    if(traitValues[GROWTH] * maxSize.x > boundingBox.w) boundingBox.w = traitValues[GROWTH] * maxSize.x;
    if(traitValues[GROWTH] * maxSize.y > boundingBox.h) boundingBox.h = traitValues[GROWTH] * maxSize.y;
}

std::array<SDL_Vertex, 3> Organism::getVelocityDirectionTriangleCoords() const {
    Vec2 normVelocity = velocity.getNormalizedVector();
    std::array<SDL_Vertex, 3> result{};
    Vec2 centerPoint{0,0}, leftPoint{0,0}, rightPoint{0,0};
    float size = 5.0f;

    if(std::abs(normVelocity.x) > std::abs(normVelocity.y)) { //x-axis
        if(std::signbit(normVelocity.x)) { //left
            centerPoint = {boundingBox.x - size, boundingBox.y + (boundingBox.h / 2.0f)};
            leftPoint = {boundingBox.x, boundingBox.y + boundingBox.h};
            rightPoint = {boundingBox.x, boundingBox.y};
        }else { //right
            centerPoint = {(boundingBox.x + boundingBox.w) + size, boundingBox.y + (boundingBox.h / 2.0f)};
            leftPoint = {boundingBox.x + boundingBox.w, boundingBox.y};
            rightPoint = {boundingBox.x + boundingBox.w, boundingBox.y + boundingBox.h};
        }
    }else { //y-axis
        if(std::signbit(normVelocity.y)) { //top
            centerPoint = {boundingBox.x + (boundingBox.w / 2.0f), boundingBox.y - size};
            leftPoint = {boundingBox.x, boundingBox.y};
            rightPoint = {boundingBox.x + boundingBox.w, boundingBox.y};
        }else { //bottom
            centerPoint = {boundingBox.x + (boundingBox.w / 2.0f), boundingBox.y + boundingBox.h + size};
            leftPoint = {boundingBox.x + boundingBox.w, boundingBox.y + boundingBox.h};
            rightPoint = {boundingBox.x, boundingBox.y + boundingBox.h};
        }
    }

    result = {
            SDL_Vertex{ //center
                .position = {
                    .x = centerPoint.x,
                    .y = centerPoint.y
                },
                .color = {
                    .r = 0.0f,
                    .g = 0.0f,
                    .b = 0.0f,
                    .a = 0.50f,
                }
            },
            SDL_Vertex{ //left
                .position = {
                    .x = leftPoint.x,
                    .y = leftPoint.y
                },
                .color = {
                    .r = 0.0f,
                    .g = 0.0f,
                    .b = 0.0f,
                    .a = 0.50f,
                }
            },
            SDL_Vertex{ //right
                .position = {
                    .x = rightPoint.x,
                    .y = rightPoint.y
                },
                .color = {
                    .r = 0.0f,
                    .g = 0.0f,
                    .b = 0.0f,
                    .a = 0.50f,
                }
            },
    };
    return result;
}

void Organism::render(SDL_Renderer* renderer) const {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    const SDL_FRect organismRect = {boundingBox.x, boundingBox.y, boundingBox.w, boundingBox.h};
    SDL_RenderFillRect(renderer, &organismRect);

    std::array<SDL_Vertex, 3> directionTriangle = getVelocityDirectionTriangleCoords();
    std::array<int, 3> verticesOrder = {1, 0, 2};
    if(!SDL_RenderGeometry(
            renderer,
            NULL,
            directionTriangle.data(),
            directionTriangle.size(),
            verticesOrder.data(),
            verticesOrder.size()))
        SDL_Log("%s", SDL_GetError());

    if(emitDangerPheromone) {
        std::uniform_int_distribution<int> distX(boundingBox.x - 20, boundingBox.x + boundingBox.w + 20);
        std::uniform_int_distribution<int> distY(boundingBox.y - 20, boundingBox.y + boundingBox.h + 20);

        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
        for(int i = 0; i < 20; i++) {
            const SDL_FRect pheromoneRect = {static_cast<float>(distX(mt)), static_cast<float>(distY(mt)), 2, 2};
            SDL_RenderFillRect(renderer, &pheromoneRect);
        }
    }
}

void Organism::handleTimer(const float deltaTime) {
    if(timer >= 1.00f) {
        hunger -= hungerStep;
        age++;

        if(age >= reproductionAge && energy >= 200) {
            std::bernoulli_distribution distReproduceChance(traitValues[Traits::FERTILITY]);
            //if(distReproduceChance(mt))
                canReproduce = true;
        }
        if(hunger <= 0 || (age >= maxAge)) {
            markedForDeletion = true;
            boundingBox.w = 12.0f;
            boundingBox.h = 12.0f;
            color = {255, 0, 0, 255};
        }

        timer = 0.0f;
    }else timer += deltaTime;
    if(growthTimer >= 5.0f) {
        if(energy >= growthEnergyThreshold) grow();
        growthTimer = 0.0f;
    }else growthTimer += deltaTime;
}

void Organism::fixedUpdate() {
    velocity.x *= velocityDecay;
    velocity.y *= velocityDecay;
}

void Organism::updateInputs() {
    std::vector<std::pair<NeuronInputType, float>> activations = neuralNet.getInputActivations();
    for(auto & [neuronID, activation] : activations) {
        switch(neuronID) {
            case HUNGER:
                activation = ((-0.01f) * static_cast<float>(hunger)) + 1;
                break;
            case BOUNDS_LEFT:
            case BOUNDS_RIGHT:
            case BOUNDS_UP:
            case BOUNDS_DOWN:
                activation = checkBounds(neuronID);
                break;
            case FOOD_LEFT:
            case FOOD_RIGHT:
            case FOOD_UP:
            case FOOD_DOWN:
                activation = std::max(findNearby<FoodSpawnRange>(neuronID), findNearby<FoodSpawnRange>(neuronID, true));
                break;
            case FOOD_COLLISION:
                activation = isColliding<Food>() ? 1.0f : 0.0f;
                break;
            case ORGANISM_LEFT:
            case ORGANISM_RIGHT:
            case ORGANISM_UP:
            case ORGANISM_DOWN:
                activation = std::max(findNearby<Organism>(neuronID), findNearby<Organism>(neuronID, true));
                break;
            case ORGANISM_COLLISION:
                activation = isColliding<Organism>() ? 1.0 : 0.0f;
                break;
            case FIRE_LEFT:
            case FIRE_RIGHT:
            case FIRE_UP:
            case FIRE_DOWN:
                activation = std::max(findNearby<Fire>(neuronID), findNearby<Fire>(neuronID, true));
                break;
            case DETECT_DANGER_PHEROMONE: { //this may not work a lot of the time, no guarantee organism detection neurons will be before pheromone detection neurons in vector.
                if(detectedDangerPheromone) {
                    activation = 1.0f;
                    detectedDangerPheromone = false;
                }else {
                    activation = 0.0f;
                }
                break;
            }
            default:
                activation = 0.00f;
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
                moveVelocity.x += -activation * acceleration * traitValues[Traits::SPEED] * deltaTime;
                move(moveVelocity);
                break;
            }
            case MOVE_RIGHT: {
                Vec2 moveVelocity = velocity;
                moveVelocity.x += activation * acceleration * traitValues[Traits::SPEED] * deltaTime;
                move(moveVelocity);
                break;
            }
            case MOVE_UP: {
                Vec2 moveVelocity = velocity;
                moveVelocity.y += -activation * acceleration * traitValues[Traits::SPEED] * deltaTime;
                move(moveVelocity);
                break;
            }
            case MOVE_DOWN: {
                Vec2 moveVelocity = velocity;
                moveVelocity.y += activation * acceleration * traitValues[Traits::SPEED] * deltaTime;
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

template<typename SimObjectType>
bool Organism::isColliding() const {
    for(const auto collisionID : collisionIDs) {
        std::shared_ptr<SimObject> basePtr = (*simState.getFuncPtr)(collisionID);
        if(basePtr) {
            SimObject& baseObject = *basePtr;
            if(typeid(SimObjectType) == typeid(baseObject)) return true;
        }
    }
    return false;
}

template<typename SimObjectType>
float Organism::findNearby(NeuronInputType neuronID, const bool useRaycast) {
    if(useRaycast && raycastNeighbors.empty()) return 0.0f;
    if(!useRaycast && neighbors.empty()) return 0.0f;
    const std::vector<std::pair<uint64_t, Vec2>>* searchObjectsPtr;

    if(useRaycast) searchObjectsPtr = &raycastNeighbors;
    else searchObjectsPtr = &neighbors;

    float distance = NAN;

    for(const auto& [neighborID, neighborDistance] : *searchObjectsPtr) {
        std::shared_ptr<SimObject> basePtr = (*simState.getFuncPtr)(neighborID);
        std::shared_ptr<SimObjectType> derivedPtr = std::dynamic_pointer_cast<SimObjectType>(basePtr);
        if(!derivedPtr) continue;
        if constexpr(std::is_same_v<SimObjectType, Organism>) {
            if(derivedPtr->isEmittingDangerPheromone())
                detectedDangerPheromone = true;
        }
        switch(neuronID) {
            case ORGANISM_LEFT:
            case FOOD_LEFT:
            case FIRE_LEFT:
                if(neighborDistance.x >= 0.0f) continue;
                distance = -neighborDistance.x;
                break;
            case ORGANISM_RIGHT:
            case FOOD_RIGHT:
            case FIRE_RIGHT:
                if(neighborDistance.x <= 0.0f) continue;
                distance = neighborDistance.x;
                break;
            case ORGANISM_UP:
            case FOOD_UP:
            case FIRE_UP:
                if(neighborDistance.y >= 0.0f) continue;
                distance = -neighborDistance.y;
                break;
            case ORGANISM_DOWN:
            case FOOD_DOWN:
            case FIRE_DOWN:
                if(neighborDistance.y <= 0.0f) continue;
                distance = neighborDistance.y;
                break;
            default: return 0.0f;
        }
        break; //if we make it here the distance should always be set
    }
    if(std::isnan(distance)) return 0.0f;

    if constexpr(std::is_same_v<SimObjectType, Fire>) {
        if(!useRaycast) emitDangerPheromone = distance <= 20.0f;
    }

    return inverseActivation(distance, useRaycast ? 0.007f : 0.05f);
}

float Organism::checkBounds(NeuronInputType neuronID) const {
    float distance;

    switch(neuronID) {
        case BOUNDS_LEFT:
            distance = boundingBox.x - static_cast<float>(simState.simBoundsPtr->x);
            break;
        case BOUNDS_RIGHT:
            distance = static_cast<float>(simState.simBoundsPtr->x + simState.simBoundsPtr->w) - (boundingBox.x + boundingBox.w);
            break;
        case BOUNDS_UP:
            distance = boundingBox.y - static_cast<float>(simState.simBoundsPtr->y);
            break;
        case BOUNDS_DOWN:
            distance = static_cast<float>(simState.simBoundsPtr->y + simState.simBoundsPtr->h) - (boundingBox.y + boundingBox.h);
            break;
        default:
            return 0.0f;
    }
    if(distance < 0.0f) distance = 0.0f;

    return inverseActivation(distance, 0.05f);
}

void Organism::tryEat(const float activation) {
    const int threshold = static_cast<int>(activation * 100.0f);
    for (const uint64_t collisionID : collisionIDs) {
        std::shared_ptr<SimObject> objectPtr = (*simState.getFuncPtr)(collisionID);
        std::shared_ptr<Food> foodPtr = std::dynamic_pointer_cast<Food>(objectPtr);
        if (foodPtr && !foodPtr->shouldDelete() && hunger < threshold) {
            hunger += foodPtr->getNutritionalValue();
            energy += foodPtr->getNutritionalValue();
            if (hunger > 100) hunger = 100;
            foodPtr->markForDeletion();
        }
    }
}
