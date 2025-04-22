#include "Simulation.hpp"
#include "QuadTree.hpp"
#include "UIStructs.hpp"
#include "Organism.hpp"
#include "StaticSimObjects.hpp"
#include "UtilityStructs.hpp"
#include "SDL3/SDL.h"
#include <cmath>
#include <sstream>
#include <iomanip>
#include <algorithm>

std::mt19937 Simulation::mt{std::random_device{}()};

Simulation::Simulation(SDL_Renderer* rendererPtr, const SDL_Rect& simBounds, const uint16_t maxPopulation, const int genomeSize, const float initialMutationFactor = 0.25f) :
    rendererPtr(rendererPtr),
    simBoundsPtr(std::make_shared<SDL_Rect>(simBounds)),
    maxPopulation(maxPopulation),
    maxFood(maxPopulation),
    mutationFactor((initialMutationFactor >= 0.0f && initialMutationFactor <= 1.0f) ? initialMutationFactor : 0.25f),
    quadTreePtr(std::make_shared<QuadTree>(rectToFRect(simBounds), 10)),
    simState(&markForDeletionLambda, &getLambda, quadTreePtr, simBoundsPtr),
    foodSpawnRange(SDL_Rect{(simBoundsPtr->x + simBoundsPtr->w) - 150, simBoundsPtr->y, 150, simBoundsPtr->h})
{
    static SDL_Color color = {50, 0, 240, 255};

    for (int i = 0; i < maxPopulation; i++) {
        const uint64_t id = getRandomID();
        const Vec2 initialPosition = getRandomPoint();
        const SDL_FRect boundingBox{
            initialPosition.x, initialPosition.y, organismWidth, organismHeight
        };
        addOrganism(id, genomeSize, color, boundingBox);
        color.r += 10;
        color.b += 15;
    }
    addFood();
    generateHeatMap();
    generateAtmosphereMap();
    startWorkerThread();
}

Simulation::~Simulation() {
    if(heatMapTexture) SDL_DestroyTexture(heatMapTexture);
    if(atmosphereMapTexture) SDL_DestroyTexture(atmosphereMapTexture);
    stopWorkerThread();
}

SDL_Color Simulation::heatValToColor(uint8_t heatVal) {
    SDL_Color color{255, 255, 255, 100};

    if(heatVal >= 120 && heatVal <= 128) {
        return color;
    }else if(heatVal > 128) {
        color.g -= heatVal;
        color.b -= heatVal;
    }else {
        color.r = heatVal;
        color.g = heatVal;
    }

    return color;
}

SDL_Color Simulation::atmosphereValToColor(uint8_t atmosphereVal) {
    SDL_Color color{255, 255, 255, 100};
    float atmosphereValF = static_cast<float>(atmosphereVal) / 255.0f;

    if(atmosphereVal >= 120 && atmosphereVal <= 128) {
        return color;
    }else if(atmosphereVal > 128) {
        color.r = 0;
        color.g = static_cast<uint8_t>(100 * atmosphereValF);
        color.b = static_cast<uint8_t>(255 * atmosphereValF);
    }else {
        atmosphereValF += 0.5f;
        color.r = static_cast<uint8_t>(50 * atmosphereValF);
        color.g = static_cast<uint8_t>(255 * atmosphereValF);
        color.b = 0;
    }
    return color;
}

void Simulation::generateAtmosphereMap() {
    if(atmosphereMapTexture) SDL_DestroyTexture(atmosphereMapTexture);
    if(!atmosphereMap.empty()) atmosphereMap.clear();
    atmosphereMapTexture = SDL_CreateTexture(rendererPtr, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, simBoundsPtr->w, simBoundsPtr->h);
    std::uniform_int_distribution<uint8_t> distAtmosphere(0, UINT8_MAX);

    SDL_SetRenderTarget(rendererPtr, atmosphereMapTexture);
    SDL_SetRenderDrawColor(rendererPtr, 255, 255, 255, 100);
    SDL_RenderClear(rendererPtr);
    for(int x = simBoundsPtr->x; x < simBoundsPtr->x + simBoundsPtr->w; x += atmosphereMapGridSize) { //if simbounds !start at 0 we could miss factors organisms may hash to
        for(int y = simBoundsPtr->y; y < simBoundsPtr->y + simBoundsPtr->h; y += atmosphereMapGridSize) {
            Vec2 position(static_cast<float>(x), static_cast<float>(y), atmosphereMapGridSize);
            SDL_FRect rect{position.x - static_cast<float>(simBoundsPtr->x), position.y - static_cast<float>(simBoundsPtr->y), atmosphereMapGridSize, atmosphereMapGridSize};
            uint8_t atmosphereVal = distAtmosphere(mt);
            SDL_Color color = atmosphereValToColor(atmosphereVal);
            atmosphereMap.insert(std::make_pair(position, atmosphereVal));
            SDL_SetRenderDrawColor(rendererPtr, color.r, color.g, color.b, color.a);
            SDL_RenderFillRect(rendererPtr, &rect);
        }
    }
    SDL_SetRenderTarget(rendererPtr, NULL);
}

void Simulation::generateHeatMap() {
    if(heatMapTexture) SDL_DestroyTexture(heatMapTexture);
    if(!heatMap.empty()) heatMap.clear();
    heatMapTexture = SDL_CreateTexture(rendererPtr, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, simBoundsPtr->w, simBoundsPtr->h);
    std::uniform_int_distribution<uint8_t> distHeat(0, UINT8_MAX);

    SDL_SetRenderTarget(rendererPtr, heatMapTexture);
    SDL_SetRenderDrawColor(rendererPtr, 255, 255, 255, 100);
    SDL_RenderClear(rendererPtr);
    for(int x = 0; x < simBoundsPtr->x + simBoundsPtr->w; x += heatMapGridSize) {
        for(int y = 0; y < simBoundsPtr->y + simBoundsPtr->h; y += heatMapGridSize) {
            Vec2 position(static_cast<float>(x), static_cast<float>(y), heatMapGridSize);
            SDL_FRect rect{position.x - static_cast<float>(simBoundsPtr->x), position.y - static_cast<float>(simBoundsPtr->y), heatMapGridSize, heatMapGridSize};
            uint8_t heatVal = distHeat(mt);
            SDL_Color color = heatValToColor(heatVal);
            heatMap.insert(std::make_pair(position, heatVal));
            SDL_SetRenderDrawColor(rendererPtr, color.r, color.g, color.b, color.a);
            SDL_RenderFillRect(rendererPtr, &rect);
        }
    }
    SDL_SetRenderTarget(rendererPtr, NULL);
}

SDL_FColor Simulation::colorToFColor(const SDL_Color& color) {
    float range = 255.0f;

    return {
        .r = static_cast<float>(color.r) / range,
        .g = static_cast<float>(color.g) / range,
        .b = static_cast<float>(color.b) / range,
        .a = static_cast<float>(color.a) / range,
    };
}

Vec2 Simulation::getRandomPoint() const {
    std::uniform_int_distribution<int> distX(simBoundsPtr->x, simBoundsPtr->x + simBoundsPtr->w);
    std::uniform_int_distribution<int> distY(simBoundsPtr->y, simBoundsPtr->y + simBoundsPtr->h);
    return {static_cast<float>(distX(mt)), static_cast<float>(distY(mt))};
}

uint64_t Simulation::getRandomID() {
    static std::uniform_int_distribution<uint64_t> distID(0, UINT64_MAX - 1);
    return distID(mt);
}
bool Simulation::shouldMutate() const {
    static std::bernoulli_distribution distBool(mutationFactor);
    return distBool(mt);
}

void Simulation::render() {
    SDL_FRect simBoundsFRect = rectToFRect(*simBoundsPtr);
    if(heatMapVisible) SDL_RenderTexture(rendererPtr, heatMapTexture, NULL, &simBoundsFRect);
    if(atmosphereMapVisible) SDL_RenderTexture(rendererPtr, atmosphereMapTexture, NULL, &simBoundsFRect);
    if(currUserAction == UserActionType::CHANGE_FOOD_RANGE) {
        SDL_SetRenderDrawColor(rendererPtr, 255, 255, 0, 100);
        SDL_FRect foodSpawnRangeFloat = rectToFRect(foodSpawnRange);
        SDL_RenderFillRect(rendererPtr, &foodSpawnRangeFloat);
    }
    for(const auto & [id, objectPtr]: simObjects) {
        objectPtr->render(rendererPtr);
    }
    if(quadTreeVisible) quadTreePtr->show(rendererPtr);
}

void Simulation::update(const SDL_Rect& newSimBounds, const float deltaTime) {
    updateSimBounds(newSimBounds);
    currUserActionFunc();

    if(paused) return;

    handleTimers(deltaTime);

    for(auto itr = simObjects.begin(); itr != simObjects.end();) {
        const uint64_t id = itr->first;
        std::shared_ptr<SimObject> objectPtr = itr->second;
        std::shared_ptr<Organism> organismPtr = nullptr;

        if(organisms.contains(id)) {
            organismPtr = organisms[id];
            setMapVals(organismPtr);
        }

        objectPtr->update(deltaTime);

        if(organisms.contains(id)) {
            organismPtr->clearCollisionIDs();
            tryAddParent(organismPtr);
        }
        checkBounds(objectPtr);

        SDL_LockMutex(workerMutex);
        if (objectPtr->shouldDelete()) {
            quadTreePtr->remove(QuadTree::QuadTreeObject(id, objectPtr->getBoundingBox()));
            if (organismPtr) {
                organisms.erase(id);
                population--;
            }
            const auto foodPtr = std::dynamic_pointer_cast<Food>(objectPtr);
            if (foodPtr) {
                auto range = foodMap.equal_range(foodPtr->getPosition());
                for(auto foodItr = range.first; foodItr != range.second; ++foodItr) {
                    if(foodPtr->getID() == foodItr->second->getID()) {
                        foodMap.erase(foodItr);
                        break;
                    }
                }
                for(const auto& [foodSpawnAreaID, foodSpawnAreaPtr] : foodSpawnRanges) {
                    if(QuadTree::rangeIntersectsRect(foodPtr->getBoundingBox(), foodSpawnAreaPtr->getBoundingBox())) {
                        foodSpawnAreaPtr->decreaseFoodAmount();
                    }
                }
                foodAmount--;
            }
            itr = simObjects.erase(itr);
        }else {
            ++itr;
        }
        SDL_UnlockMutex(workerMutex);
    }

    for(const auto& [id1, id2] : quadTreePtr->getIntersections()) {
        handleCollision(id1, id2);
    }
}

void Simulation::tryAddParent(const std::shared_ptr<Organism> &organismPtr) {
    if (
        organismPtr->shouldReproduce() &&
        std::find(nextGenParents.begin(), nextGenParents.end(), organismPtr) == nextGenParents.end()
    ) {
        nextGenParents.push_back(organismPtr);
    }
}

void Simulation::setMapVals(const std::shared_ptr<Organism>& organismPtr) {
    const Vec2 organismPositionFoodMap = organismPtr->getPosition();
    const Vec2 organismPositionHeatMap(organismPositionFoodMap.x, organismPositionFoodMap.y, heatMapGridSize);
    const Vec2 organismPositionAtmosphereMap(organismPositionFoodMap.x, organismPositionFoodMap.y, atmosphereMapGridSize);
    if(foodMap.contains(organismPositionFoodMap)) {
        auto range = foodMap.equal_range(organismPositionFoodMap);
        for(auto foodItr = range.first; foodItr != range.second; ++foodItr){
            organismPtr->addCollisionID(foodItr->second->getID());
        }
        slowInFood(organismPtr);
    }
    if(heatMap.contains(organismPositionHeatMap))
        organismPtr->setTemperature(heatMap[organismPositionHeatMap]);
    else SDL_Log("No heat map value for organism position");
    if(atmosphereMap.contains(organismPositionAtmosphereMap)) {
        const uint8_t atmosphereVal = atmosphereMap[organismPositionAtmosphereMap];
        if(atmosphereVal > 128) {
            organismPtr->setOxygenSat(static_cast<float>(atmosphereVal - 128) / 127.0f);
            organismPtr->setHydrogenSat(0.0f);
        }else {
            organismPtr->setHydrogenSat(static_cast<float>(atmosphereVal) / 128.0f);
            organismPtr->setOxygenSat(0.0f);
        }
    }else SDL_Log("No atmosphere map value for organism position");
}

void Simulation::startWorkerThread() {
    workerMutex = SDL_CreateMutex();
    workerCondition = SDL_CreateCondition();

    threadData = std::make_shared<ThreadData>(
        [this](){
            while(true) {
                SDL_LockMutex(workerMutex);

                while(!workAvailable && workerRunning) {
                    SDL_WaitCondition(workerCondition, workerMutex);
                }

                if(!workerRunning) {
                    SDL_UnlockMutex(workerMutex);
                    break;
                }

                neighborTask();
                workAvailable = false;
                SDL_UnlockMutex(workerMutex);
            }
        }
    );

    workerThread = SDL_CreateThread(
            [](void* data) -> int{
                auto* td(static_cast<ThreadData*>(data));
                td->threadFunc();
                return 0;
            },
            "NeighborsThread", static_cast<void*>(threadData.get()));
}

void Simulation::stopWorkerThread() {
    SDL_LockMutex(workerMutex);
    workerRunning = false;
    SDL_SignalCondition(workerCondition);
    SDL_UnlockMutex(workerMutex);

    if(workerThread) SDL_WaitThread(workerThread, nullptr);
    if(workerMutex) SDL_DestroyMutex(workerMutex);
    if(workerCondition) SDL_DestroyCondition(workerCondition);
    workerThread = nullptr, workerMutex = nullptr, workerCondition = nullptr;
}

void Simulation::neighborTask() {
    if(!workerThreadQuadTreeCopy) return;

    for(auto & [id, objectPtr]: simObjects) {
        objectPtr->fixedUpdate();
        if(organisms.contains(id)) {
            const std::shared_ptr<Organism> organismPtr = organisms[id];
            std::vector<std::pair<uint64_t, Vec2>> neighbors = workerThreadQuadTreeCopy->getNearestNeighbors(
                    QuadTree::QuadTreeObject(
                            organismPtr->getID(),
                            organismPtr->getBoundingBox()));
            std::vector<std::pair<uint64_t, Vec2>> raycastNeighbors = workerThreadQuadTreeCopy->raycast(
                    QuadTree::QuadTreeObject(
                            organismPtr->getID(),
                            organismPtr->getBoundingBox()), organismPtr->getVelocity());
            organismPtr->addNeighbors(neighbors);
            organismPtr->addRaycastNeighbors(raycastNeighbors);
        }
    }
}

void Simulation::fixedUpdate() {
    if(paused) return;
    static int calls = 6;
    SDL_LockMutex(workerMutex);
    quadTreePtr->undivide();
    if(calls >= 6) {
        workerThreadQuadTreeCopy = std::make_unique<QuadTree>(*quadTreePtr);
        calls = 0;
    }else calls++;
    workAvailable = true;
    SDL_SignalCondition(workerCondition);
    SDL_UnlockMutex(workerMutex);
}

void Simulation::randomizeFoodParams() {
    std::uniform_int_distribution<int> distX(simBoundsPtr->x, (simBoundsPtr->x + simBoundsPtr->w));
    std::uniform_int_distribution<int> distY(simBoundsPtr->y, (simBoundsPtr->y + simBoundsPtr->h));

    std::uniform_int_distribution<int> distFoodAmount(50, 100);
    foodSpawnAmount = distFoodAmount(mt);

    int foodSpawnRangeX1 = distX(mt), foodSpawnRangeX2 = distX(mt) , foodSpawnRangeY1 = distY(mt), foodSpawnRangeY2 = distY(mt);
    int foodSpawnRangeXMin = std::min(foodSpawnRangeX1, foodSpawnRangeX2), foodSpawnRangeXMax = std::max(foodSpawnRangeX1, foodSpawnRangeX2);
    int foodSpawnRangeYMin = std::min(foodSpawnRangeY1, foodSpawnRangeY2), foodSpawnRangeYMax = std::max(foodSpawnRangeY1, foodSpawnRangeY2);
    int foodSpawnRangeWidth = foodSpawnRangeXMax - foodSpawnRangeXMin, foodSpawnRangeHeight = foodSpawnRangeYMax - foodSpawnRangeYMin;
    if(foodSpawnRangeWidth < 80 || foodSpawnRangeHeight < 80) return;
    if(foodSpawnRangeWidth > 150) foodSpawnRangeWidth = 150;
    if(foodSpawnRangeHeight > 150) foodSpawnRangeHeight = 150;

    foodSpawnRange = {foodSpawnRangeXMin, foodSpawnRangeYMin, foodSpawnRangeWidth, foodSpawnRangeHeight};
}

void Simulation::handleTimers(const float deltaTime) {
    if(foodTimer >= 10.0f) {
        addFood();
        addFire();
        foodTimer = 0.0f;
    }else foodTimer += deltaTime;
    if(foodRandomizeTimer >= 30.0f) {
        if(foodSpawnRandom) randomizeFoodParams();
        foodRandomizeTimer = 0.0f;
    }else foodRandomizeTimer += deltaTime;
    if(generationTimer >= generationLength) {
        if(population < 100) {
            birthRate = std::make_pair(10, 20);
        }else {
            birthRate = std::make_pair(2, 3);
        }
        createNextGeneration();
        generationTimer = 0.0f;
    }else generationTimer += deltaTime;
    if(mutationTimer >= 5.0f) {
        mutateOrganisms();
        mutationTimer = 0.0f;
    }else mutationTimer += deltaTime;
}

void Simulation::mutateOrganisms() {
    for(const auto& [id, organismPtr] : organisms) {
        if(shouldMutate())
            organismPtr->mutateGenome();
    }
}

void Simulation::addFire() {
    if(fireAmount >= maxFires) return;
    std::uniform_int_distribution<int> distX(simBoundsPtr->x, (simBoundsPtr->x + simBoundsPtr->w) - 100);
    std::uniform_int_distribution<int> distY(simBoundsPtr->y, (simBoundsPtr->y + simBoundsPtr->h) - 100);

    auto x = static_cast<float>(distX(mt)), y = static_cast<float>(distY(mt));
    const SDL_FRect boundingBox{x, y, 100, 100};
    for(const auto& [foodSpawnRangeID, foodSpawnRangePtr] : foodSpawnRanges) {
        if(QuadTree::rangeIntersectsRect(boundingBox, foodSpawnRangePtr->getBoundingBox())) return;
    }

    const uint64_t id = getRandomID();
    SDL_Color color{252, 119, 3, 255};
    addSimObject(std::make_shared<Fire>(id, boundingBox, color, simState));
    fireAmount++;
}

void Simulation::addFood() {
    uint16_t foodSpawnAmountLocal = foodSpawnAmount;
    if(foodAmount + foodSpawnAmount > maxFood) {
        foodSpawnAmountLocal = foodSpawnAmount - foodAmount;
    }
    if(
        foodSpawnRange.x < simBoundsPtr->x ||
        (foodSpawnRange.x + foodSpawnRange.w) > (simBoundsPtr->x + simBoundsPtr->w) ||
        foodSpawnRange.y < simBoundsPtr->y ||
        (foodSpawnRange.y + foodSpawnRange.h) > (simBoundsPtr->y + simBoundsPtr->h)
    ) return;
    foodAmount += foodSpawnAmountLocal;

    std::uniform_int_distribution<int> distX(foodSpawnRange.x, (foodSpawnRange.x + foodSpawnRange.w) - (int)foodWidth);
    std::uniform_int_distribution<int> distY(foodSpawnRange.y, (foodSpawnRange.y + foodSpawnRange.h) - (int)foodHeight);

    for(int i = 0; i < foodSpawnAmountLocal; i++) {
        const SDL_FRect foodBoundingBox{
                static_cast<float>(distX(mt)), static_cast<float>(distY(mt)), foodWidth, foodHeight
        };
        SDL_Color color{0, 255, 0, 200};
        const uint64_t foodID = getRandomID();
        const auto foodPtr = std::make_shared<Food>(
                foodID,
                foodBoundingBox,
                color,
                100,
                simState);
        addSimObject(foodPtr, false);
        foodMap.insert(std::make_pair(foodPtr->getPosition(), foodPtr));
    }
    addFoodSpawnRange();
}

void Simulation::addSimObject(const std::shared_ptr<SimObject>& simObjectPtr, const bool addToQuadTree) {
    if(addToQuadTree) quadTreePtr->insert(QuadTree::QuadTreeObject(simObjectPtr->getID(), simObjectPtr->getBoundingBox()));
    simObjects.insert(std::make_pair(simObjectPtr->getID(), simObjectPtr));
}

void Simulation::addFoodSpawnRange() {
    const uint64_t id = getRandomID();
    foodSpawnRanges.emplace(id, std::move(std::make_shared<FoodSpawnRange>(id, rectToFRect(foodSpawnRange), foodSpawnAmount, simState)));
    addSimObject(foodSpawnRanges[id], false);
    quadTreePtr->insert(QuadTree::QuadTreeObject(id, rectToFRect(foodSpawnRange), true));
}

void Simulation::addOrganism(
        const uint64_t id,
        const uint16_t genomeSize,
        const SDL_Color& initialColor,
        const SDL_FRect& boundingBox) {

    organisms.emplace(id, std::move(std::make_shared<Organism>(id, genomeSize, initialColor, boundingBox, simState)));
    addSimObject(organisms[id]);

    population++;
}
void Simulation::addOrganism(
        const uint64_t id,
        const Organism& parent1,
        const Organism& parent2,
        const SDL_Color& initialColor,
        const SDL_FRect& boundingBox) {

    organisms.emplace(id, std::move(std::make_shared<Organism>(id, parent1, parent2, initialColor, boundingBox, simState)));
    addSimObject(organisms[id]);

    population++;
}

void Simulation::createNextGeneration() {
    size_t size = nextGenParents.size();
    if(size < 2) return;

    for(int i = 0; i < size - 1; i += 2)
        reproduceOrganisms(nextGenParents[i], nextGenParents[i + 1]);

    if((size & 1) != 0)
        reproduceOrganisms(nextGenParents[size - 2], nextGenParents[size - 1]);

    generationNum++;

    nextGenParents.clear();
}

void Simulation::slowInFood(const std::shared_ptr<Organism>& organismPtr) {
    Vec2 velocity = organismPtr->getVelocity();
    organismPtr->setVelocity({velocity.x * 0.80f,velocity.y * 0.80f});
}

void Simulation::reproduceOrganisms(const std::shared_ptr<Organism>& organism1Ptr, const std::shared_ptr<Organism>& organism2Ptr) {
    static SDL_Color color = {50, 0, 240, 255};
    if(population + birthRate.second >= maxPopulation) return;
    if(!organism1Ptr->shouldReproduce() || !organism2Ptr->shouldReproduce()) return;

    std::uniform_int_distribution<uint8_t> distNumChildren(birthRate.first, birthRate.second);
    std::uniform_int_distribution<int> distX(simBoundsPtr->x, (simBoundsPtr->x + simBoundsPtr->w) - (int)organismWidth);
    std::uniform_int_distribution<int> distY(simBoundsPtr->y, (simBoundsPtr->y + simBoundsPtr->h) - (int)organismHeight);
    const uint8_t numChildren = distNumChildren(mt);

    organism1Ptr->reproduce();
    organism2Ptr->reproduce();
    for(int i = 0; i < numChildren; i++) {
        const uint64_t newID = getRandomID();
        addOrganism(
            newID,
            *organism1Ptr,
            *organism2Ptr,
            color,
            {
                randomizeSpawn ? static_cast<float>(distX(mt)) : organism1Ptr->getPosition().x,
                randomizeSpawn ? static_cast<float>(distY(mt)) : organism1Ptr->getPosition().y,
                organismWidth,
                organismHeight
            }
        );
        color.r += 10;
        color.b += 15;
    }
}

void Simulation::updateSimBounds(const SDL_Rect& newSimBounds) {
    if(simBoundsPtr->x != newSimBounds.x ||
       simBoundsPtr->y != newSimBounds.y ||
       simBoundsPtr->w != newSimBounds.w ||
       simBoundsPtr->h != newSimBounds.h) {

        *simBoundsPtr = newSimBounds;
        *quadTreePtr = QuadTree(rectToFRect(*simBoundsPtr), 10);
        generateHeatMap();
    }
}

void Simulation::checkBounds(const std::shared_ptr<SimObject>& objectPtr) const{
    const SDL_FRect simBoundsFloat = rectToFRect(*simBoundsPtr);
    const auto leftBound = simBoundsFloat.x;
    const auto rightBound = simBoundsFloat.x + simBoundsFloat.w;
    const auto topBound = simBoundsFloat.y;
    const auto bottomBound = simBoundsFloat.y + simBoundsFloat.h;
    SDL_FRect boundingBox = objectPtr->getBoundingBox();
    SDL_FRect oldBoundingBox = boundingBox;

    if(boundingBox.x < leftBound)
        boundingBox.x = leftBound;
    if(boundingBox.x + boundingBox.w > rightBound)
        boundingBox.x = rightBound - boundingBox.w;
    if(boundingBox.y < topBound)
        boundingBox.y = topBound;
    if(boundingBox.y + boundingBox.h > bottomBound)
        boundingBox.y = bottomBound - boundingBox.h;

    if(boundingBox.x != oldBoundingBox.x || boundingBox.y != oldBoundingBox.y) {
        objectPtr->markForDeletion(); //todo maybe remove
        quadTreePtr->remove(QuadTree::QuadTreeObject(objectPtr->getID(), oldBoundingBox));
        quadTreePtr->insert(QuadTree::QuadTreeObject(objectPtr->getID(), boundingBox));
    }
    objectPtr->setBoundingBox(boundingBox);
}

void Simulation::resolveCollision(const uint64_t id1, const uint64_t id2) {
    if(!simObjects.contains(id1) || !simObjects.contains(id2) &&
       !organisms.contains(id1)  && !organisms.contains(id2)) return;

    std::shared_ptr<SimObject> object1Ptr = simObjects[id1], object2Ptr = simObjects[id2];

    SDL_FRect boundingBox1 = object1Ptr->getBoundingBox(), boundingBox2 = object2Ptr->getBoundingBox();

    if(organisms.contains(id1) && organisms.contains(id2)) {
        Vec2 velocity1 = organisms[id1]->getVelocity(), velocity2 = organisms[id2]->getVelocity();

        //from https://www.plasmaphysics.org.uk/programs/coll2d_cpp.htm
        float mass1 = 10.0f, mass2 = 10.0f, R = 0.95f;
        float massRatio = mass2 / mass1;
        float xDiff = boundingBox2.x - boundingBox1.x, yDiff = boundingBox2.y - boundingBox1.y;
        float xVelocityDiff = velocity2.x - velocity1.x, yVelocityDiff = velocity2.y - velocity1.y;
        float xVelocityCM = (mass1 * velocity1.x + mass2 * velocity2.x) / (mass1 + mass2);
        float yVelocityCM = (mass1 * velocity1.y + mass2 * velocity2.y) / (mass1 + mass2);

        //don't update velocities if bounding boxes not approaching
        if((xVelocityDiff * xDiff + yVelocityDiff * yDiff) >= 0) return;

        float yDiffF = 1.0E-6F * std::fabs(yDiff);
        if(std::fabs(xDiff) < yDiffF) {
            float sign;
            if(xDiff < 0.0f) sign = -1.0f;
            else sign = 1.0f;
            xDiff = yDiffF * sign;
        }

        //update velocities
        float slope = yDiff / xDiff;
        float dxVelocity2 = -2.0f * (xVelocityDiff + slope * yVelocityDiff) / ((1 + slope * slope) * (1 + massRatio));
        velocity2.x = velocity2.x + dxVelocity2;
        velocity2.y = velocity2.y + slope * dxVelocity2;
        velocity1.x = velocity1.x - massRatio * dxVelocity2;
        velocity1.y = velocity1.y - slope * massRatio * dxVelocity2;

        //velocity correction for inelastic collisions
        velocity1.x = (velocity1.x - xVelocityCM) * R + xVelocityCM;
        velocity1.y = (velocity1.y - yVelocityCM) * R + yVelocityCM;
        velocity2.x = (velocity2.x - xVelocityCM) * R + xVelocityCM;
        velocity2.y = (velocity2.y - yVelocityCM) * R + yVelocityCM;

        if(abs(velocity1.x) <= Organism::velocityMax && abs(velocity1.y) <= Organism::velocityMax) {
            organisms[id1]->setVelocity(velocity1);
        }
        if(abs(velocity2.x) <= Organism::velocityMax && abs(velocity2.y) <= Organism::velocityMax) {
            organisms[id2]->setVelocity(velocity2);
        }
    }
}

void Simulation::handleCollision(const uint64_t id1, const uint64_t id2) {
    if(!simObjects.contains(id1) || !simObjects.contains(id2)) return;

    resolveCollision(id1, id2);

    std::shared_ptr<SimObject> object1Ptr(simObjects[id1]), object2Ptr(simObjects[id2]);
    if(organisms.contains(id1)) organisms[id1]->addCollisionID(id2);
    if(organisms.contains(id2)) organisms[id2]->addCollisionID(id1);
    std::shared_ptr<Fire> fire1Ptr = std::dynamic_pointer_cast<Fire>(object1Ptr), fire2Ptr = std::dynamic_pointer_cast<Fire>(object2Ptr);

    if(fire1Ptr && !fire2Ptr) object2Ptr->markForDeletion();
    if(fire2Ptr && !fire1Ptr) object1Ptr->markForDeletion();
}

SimObjectData Simulation::userClicked(const float mouseX, const float mouseY) {
    SimObjectData result{};

    std::vector<uint64_t> objectsClicked = quadTreePtr->query(
        QuadTree::QuadTreeObject(
            SDL_FRect{mouseX, mouseY, clickWidth, clickHeight}));
    std::shared_ptr<Organism> organismPtr = nullptr;

    for(const uint64_t id : objectsClicked) {
        if(organisms.contains(id)) {
            organismPtr = organisms[id];
            break;
        }
    }
    if(organismPtr) result = getOrganismData(organismPtr);

    return result;
}

SimObjectData Simulation::getFocusedSimObjectData() {
    if(focusedSimObjectID == UINT64_MAX || !simObjects.contains(focusedSimObjectID)) return {};

    const std::shared_ptr<SimObject> objectPtr = simObjects[focusedSimObjectID];

    if(organisms.contains(focusedSimObjectID)) {
        const std::shared_ptr<Organism> organismPtr = organisms[focusedSimObjectID];

        return getOrganismData(organismPtr);
    }

    return {}; //todo add more get methods for other types
}

OrganismData Simulation::getOrganismData(const std::shared_ptr<Organism>& organismPtr) {
    std::stringstream organismInfoStream;
    std::stringstream neuralNetInputStream;
    std::stringstream neuralNetOutputStream;
    std::stringstream traitInfoStream;
    std::vector<std::pair<NeuronInputType, float>> inputActivations = organismPtr->getInputActivations();
    std::vector<std::pair<NeuronOutputType, float>> outputActivations = organismPtr->getOutputActivations();

    organismInfoStream << "ID: " << organismPtr->getID() << std::endl <<
        "Hunger: " << static_cast<int>(organismPtr->getHunger()) << "%" << std::endl <<
        "Age: " << static_cast<int>(organismPtr->getAge()) << std::endl <<
        "Energy: " << static_cast<int>(organismPtr->getEnergy()) << std::endl <<
        "Temperature: " << static_cast<int>(organismPtr->getTemperature()) << "°F" << std::endl <<
        "Breath: " << static_cast<int>(organismPtr->getBreath()) << "%" << std::endl <<
        "Oxygen Sat: " << static_cast<int>(organismPtr->getOxygenSat() * 100) << "%" << std::endl <<
        "Hydrogen Sat: " << static_cast<int>(organismPtr->getHydrogenSat() * 100) << "%" << std::endl;

    neuralNetInputStream << "Neural Net Inputs: " << std::endl;
    for(const auto& [neuronID, activation] : inputActivations) {
        neuralNetInputStream << "ID: " << inputValues[neuronID] << " Activation: "
            << std::fixed << std::setprecision(2) << activation << std::endl;
    }
    neuralNetOutputStream << "Neural Net Outputs: " << std::endl;
    for(const auto& [neuronID, activation] : outputActivations) {
        neuralNetOutputStream << "ID: " << outputValues[neuronID - NEURONINPUTTYPE_SIZE] << " Activation: "
            << std::fixed << std::setprecision(2) << activation << std::endl;
    }

    const auto traitValues = organismPtr->getTraitValues();
    traitInfoStream << "Traits: " << std::endl;
    for(int i = 0; i < traitValues.size(); i++) {
        traitInfoStream << traitsStrValues[i] << " : " << traitValues[i] << std::endl;
    }

    return {
            organismPtr->getID(),
            organismPtr->getHunger(), organismPtr->getAge(),
            organismInfoStream.str(),
            neuralNetInputStream.str(),
            neuralNetOutputStream.str(),
            traitInfoStream.str()
    };
}

void Simulation::setUserAction(const UserActionType& userActionType, const UIData& uiData) {
    currUserAction = userActionType;
    currUserActionFunc = [this, userActionType, uiData] () {userActionFuncMapping[static_cast<size_t>(userActionType)](uiData);};
}


void Simulation::handleChangeFoodRange(const UIData &uiData) {
    int keyStatesLength;
    const bool* keyStates = SDL_GetKeyboardState(&keyStatesLength);
    if(keyStates[SDL_SCANCODE_RETURN]) {
        randomizeFoodParams();
        foodSpawnRandom = true;
    }
    else if(keyStates[SDL_SCANCODE_BACKSPACE]) foodSpawnRandom = false;
    if(foodSpawnRandom) return;

    static bool clickedLastFrame = false;
    static float lastFloatX = NAN, lastFloatY = NAN;
    float floatX, floatY;
    const SDL_MouseButtonFlags mouseState = SDL_GetMouseState(&floatX, &floatY);
    const bool leftClicked = mouseState & SDL_BUTTON_LMASK;
    int x = static_cast<int>(floatX);
    int y = static_cast<int>(floatY);
    if(x < simBoundsPtr->x) return;
    x = x > (simBoundsPtr->x + simBoundsPtr->w) ? (simBoundsPtr->x + simBoundsPtr->w) : x;
    y = y < simBoundsPtr->y ? simBoundsPtr->y : y;
    y = y > (simBoundsPtr->y + simBoundsPtr->h) ? (simBoundsPtr->y + simBoundsPtr->h) : y;

    if(clickedLastFrame && leftClicked) {
        const int lastX = static_cast<int>(lastFloatX);
        const int lastY = static_cast<int>(lastFloatY);
        if(lastX <= x && lastY <= y)
            foodSpawnRange = SDL_Rect{lastX, lastY, (x - lastX), (y - lastY)};
        else if(lastX > x && lastY > y)
            foodSpawnRange = SDL_Rect{x, y, (lastX - x), (lastY - y)};
        else if(lastX > x && lastY < y)
            foodSpawnRange = SDL_Rect{x, lastY, (lastX - x), (y - lastY)};
        else if(lastX < x && lastY > y)
            foodSpawnRange = SDL_Rect{lastX, y, (x - lastX), (lastY - y)};
    }
    if(leftClicked) {
        if(!clickedLastFrame) {
            lastFloatX = floatX;
            lastFloatY = floatY;
        }
        clickedLastFrame = true;
    }else clickedLastFrame = false;
}

void Simulation::handleFocus(const UIData& uiData) {
    const uint64_t* simObjectIDPtr = std::get_if<SimObjectID>(&uiData);
    if(!simObjectIDPtr || !simObjects.contains(*simObjectIDPtr)) return;
    if(focusedSimObjectID != UINT64_MAX && simObjects.contains(focusedSimObjectID)) {
        simObjects[focusedSimObjectID]->setColor({0, 0, 0, 255});
    }
    focusedSimObjectID = *simObjectIDPtr;
    simObjects[*simObjectIDPtr]->setColor({255,192, 203, 255});
    setUserAction(UserActionType::NONE, uiData);
}

void Simulation::handleUnfocus(const UIData& uiData) {
    setUserAction(UserActionType::NONE, uiData);
    if(simObjects.contains(focusedSimObjectID)) {
        simObjects[focusedSimObjectID]->setColor({0,0, 0, 255});
    }
    focusedSimObjectID = UINT64_MAX;
}