#ifndef SIMULATION_HPP
#define SIMULATION_HPP

#include "SimObject.hpp"
#include "StaticSimObjects.hpp"
#include "Organism.hpp"
#include "SimUtils.hpp"
#include "UIStructs.hpp"
#include "UtilityStructs.hpp"
#include "SDL3/SDL.h"
#include <functional>
#include <unordered_map>
#include <memory>

class Simulation{
public:
    Simulation(SDL_Renderer* rendererPtr, const SDL_Rect& simBounds, uint16_t maxPopulation, int genomeSize, float initialMutationFactor);
    ~Simulation();
    void update(const SDL_Rect& simBounds, float deltaTime);
    void fixedUpdate();
    void render();
    void setRenderer(SDL_Renderer* newRendererPtr) {rendererPtr = newRendererPtr;}

    SimObjectData userClicked(float mouseX, float mouseY);
    SimObjectData getFocusedSimObjectData();
    UserActionType getCurrentUserAction() {return currUserAction;}
    void setUserAction(const UserActionType& userActionType, const UIData& uiData);

    uint64_t getCurrentGeneration() const {return generationNum;}
    uint16_t getCurrentPopulation() const {return population;}
    void showQuadTree(bool setQuadTreeVisible) {quadTreeVisible = setQuadTreeVisible;}
    //[[nodiscard]] bool quadTreeIsShown() const {return quadTreeVisible;}
    [[nodiscard]] size_t getQuadSize() const {return quadTreePtr->size();}
    void showHeatMap(bool setHeatMapVisible) {heatMapVisible = setHeatMapVisible;}
    void showAtmosphereMap(bool setAtmosphereMapVisible) {atmosphereMapVisible = setAtmosphereMapVisible;}
    //[[nodiscard]] bool heatMapIsShown() const {return heatMapVisible;}
    bool contains(const uint64_t id) {return simObjects.contains(id);}

private:
    SDL_Renderer* rendererPtr;
    uint64_t generationNum = 0;
    uint16_t population = 0;
    const uint16_t maxPopulation;
    const uint16_t maxFood;
    const uint16_t maxPheromones;
    static constexpr uint8_t maxFires = 5;
    float foodTimer = 0.0f;
    float foodRandomizeTimer = 0.0f;
    float generationTimer = 0.0f;
    float mutationTimer = 0.0f;
    std::pair<uint8_t, uint8_t> birthRate = std::make_pair(10, 50);
    bool paused = false;
    float mutationFactor;

    UserActionType currUserAction = UserActionType::NONE;

    uint64_t focusedSimObjectID = UINT64_MAX;

    std::unordered_map<uint64_t, std::shared_ptr<SimObject>> simObjects;
    std::unordered_map<uint64_t, std::shared_ptr<Organism>> organisms;
    std::unordered_map<uint64_t, std::shared_ptr<FoodSpawnRange>> foodSpawnRanges;
    std::unordered_map<uint64_t, std::shared_ptr<Fire>> fires;
    std::unordered_multimap<Vec2, std::shared_ptr<Food>, Vec2PositionalHash, Vec2PositionalEqual> foodMap;
    std::unordered_multimap<Vec2, std::shared_ptr<Pheromone>, Vec2PositionalHash, Vec2PositionalEqual> pheromoneMap;
    std::unordered_map<Vec2, uint8_t, Vec2PositionalHash, Vec2PositionalEqual> heatMap;
    std::unordered_map<Vec2, uint8_t, Vec2PositionalHash, Vec2PositionalEqual> atmosphereMap;
    SDL_Texture* heatMapTexture = nullptr;
    SDL_Texture* atmosphereMapTexture = nullptr;
    std::vector<std::shared_ptr<Organism>> nextGenParents;
    void markForDeletion(const uint64_t id) {simObjects[id]->markForDeletion();}
    std::shared_ptr<SimObject> get(const uint64_t id) {
        if(!simObjects.contains(id)) return nullptr;
        return simObjects[id];
    }
    std::function<void (uint64_t id)> markForDeletionLambda = [this] (const uint64_t id) {this->markForDeletion(id);};
    std::function<std::shared_ptr<SimObject> (uint64_t id)> getLambda = [this] (const uint64_t id) {return this->get(id);};
    std::shared_ptr<SDL_Rect> simBoundsPtr;
    std::shared_ptr<QuadTree> quadTreePtr;
    SimUtils::SimState simState;
    SDL_Rect foodSpawnRange;
    SDL_Rect renderFoodSpawnRange;
    uint16_t foodAmount = 0;
    uint16_t pheromoneAmount = 0;
    uint16_t pheromoneSpawnAmount = 20;
    uint8_t fireAmount = 0;
    uint16_t foodSpawnAmount = 1000;
    bool foodSpawnRandom = false;
    bool randomizeSpawn = true;
    bool quadTreeVisible = false;
    bool heatMapVisible = false;
    bool atmosphereMapVisible = false;
    static constexpr float clickWidth = 8.0f;
    static constexpr float clickHeight = 8.0f;
    static constexpr float pheromoneWidth = 2.0f;
    static constexpr float pheromoneHeight = 2.0f;
    static constexpr float organismWidth = 8.0f;
    static constexpr float organismHeight = 8.0f;
    static constexpr float foodWidth = 6.0f;
    static constexpr float foodHeight = 6.0f;
    static constexpr float generationLength = 10.0f;
    static constexpr float heatMapGridSize = 200.0f;
    static constexpr float atmosphereMapGridSize = 200.0f;
    std::unique_ptr<QuadTree> workerThreadQuadTreeCopy = nullptr;
    std::shared_ptr<ThreadData> threadData = nullptr;
    SDL_Thread* workerThread = nullptr;
    SDL_Mutex* workerMutex = nullptr;
    SDL_Condition* workerCondition = nullptr;
    bool workerRunning = true;
    bool workAvailable = false;

    static SDL_Color heatValToColor(uint8_t heatVal);
    static SDL_Color atmosphereValToColor(uint8_t atmosphereVal);
    static void slowInFood(const std::shared_ptr<Organism>& organismPtr);
    static uint64_t getRandomID();
    static OrganismData getOrganismData(const std::shared_ptr<Organism>& organismPtr);

    void setMapVals(const std::shared_ptr<Organism>& organismPtr);
    void generateHeatMap();
    void generateAtmosphereMap();
    void neighborTask();
    void startWorkerThread();
    void stopWorkerThread();
    void handleTimers(float deltaTIme);
    void createNextGeneration();
    void randomizeFoodParams();
    void addPheromones(const std::shared_ptr<Organism>& organismPtr);
    void removeFromPheromoneMap(const std::shared_ptr<Pheromone>& pheromonePtr);
    void removeFromFoodMap(const std::shared_ptr<Food>& foodPtr);
    void incrementFoodSpawnRange(const SDL_FRect& foodBoundingBox);
    void decrementFoodSpawnRange(const SDL_FRect& foodBoundingBox);
    void addFire();
    uint16_t addFood();
    void addSimObject(const std::shared_ptr<SimObject>& simObjectPtr, bool isHighPriority = true);
    void addOrganism(
            uint64_t id,
            uint16_t genomeSize,
            const SDL_Color& initialColor,
            const SDL_FRect& boundingBox);
    void addOrganism(
            uint64_t id,
            const Organism& parent1,
            const Organism& parent2,
            const SDL_Color& initialColor,
            const SDL_FRect& boundingBox);
    void addFoodSpawnRange(uint16_t foodAdded);
    void tryAddParent(const std::shared_ptr<Organism>& organismPtr);
    void reproduceOrganisms(const std::shared_ptr<Organism>& organism1Ptr, const std::shared_ptr<Organism>& organism2Ptr);
    void mutateOrganisms();
    void handleCollision(uint64_t id1, uint64_t id2);
    void resolveCollision(uint64_t id1, uint64_t id2);
    void tryUpdateSimBounds(const SDL_Rect& newSimBounds);
    void checkBounds(const std::shared_ptr<SimObject>& objectPtr) const;
    bool shouldMutate() const;
    void setMutationFactor(float newMutationFactor) {
        if(newMutationFactor >= 0.0f && newMutationFactor <= 1.0f)
            mutationFactor = newMutationFactor;
    }

    void handleChangeFoodRange(const UIData& uiData);
    void handleFocus(const UIData& uiData);
    void handleUnfocus(const UIData& uiData);

    std::array<std::function<void (const UIData&)>, static_cast<size_t>(UserActionType::SIZE)> userActionFuncMapping = {
        [] (const UIData& uiData) {}, //none
        [this] (const UIData& uiData) {handleChangeFoodRange(uiData);}, //change_food_range
        [this] (const UIData& uiData) {
            paused = true;
            setUserAction(UserActionType::NONE, uiData);
        }, //pause
        [this] (const UIData& uiData) { //unpause
            paused = false;
            setUserAction(UserActionType::NONE, uiData);
        },
        [this] (const UIData& uiData) { //focus
            handleFocus(uiData);
        },
        [this] (const UIData& uiData) { //unfocus
            handleUnfocus(uiData);
        },
        [this] (const UIData& uiData) { //randomize_spawn   todo: maybe split into a randomize_spawn and parent_spawn?
            randomizeSpawn = !randomizeSpawn;
            setUserAction(UserActionType::NONE, uiData);
        }
    };

    std::function<void()> currUserActionFunc = [this] () {userActionFuncMapping[0](UIData());};
    Vec2 getRandomPoint() const;
};

#endif //SIMULATION_HPP
