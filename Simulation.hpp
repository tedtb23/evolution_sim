#ifndef SIMULATION_HPP
#define SIMULATION_HPP

#include "SimObject.hpp"
#include "Organism.hpp"
#include "SimStructs.hpp"
#include "UIStructs.hpp"
#include "SDL3/SDL.h"
#include <functional>
#include <unordered_map>
#include <memory>

class Simulation{
public:
    Simulation(const SDL_Rect& simBounds, uint16_t maxPopulation, int genomeSize, float initialMutationFactor);
    void update(const SDL_Rect& simBounds, float deltaTime);
    void fixedUpdate();
    void render(SDL_Renderer* renderer);
    void setGenomeSize(int genomeSize);

    SimObjectData userClicked(float mouseX, float mouseY);
    SimObjectData getFocusedSimObjectData();
    UserActionType getCurrentUserAction() {return currUserAction;}
    void setUserAction(const UserActionType& userActionType, const UIData& uiData);

    uint64_t getCurrentGeneration() const {return generationNum;}
    uint16_t getCurrentPopulation() const {return population;}
    void showQuadTree(bool setQuadTreeVisible) {quadTreeVisible = setQuadTreeVisible;}
    [[nodiscard]] bool quadTreeIsShown() const {return quadTreeVisible;}
    [[nodiscard]] size_t getQuadSize() const {return quadTreePtr->size();}

private:
    uint64_t generationNum = 0;
    uint16_t foodAmount = 0;
    uint16_t population = 0;
    const uint16_t maxPopulation;
    float foodTimer = 0.0f;
    float genTimer = 0.0f;
    float mutationTimer = 0.0f;
    float birthRateTimer = 0.0f;
    std::pair<uint8_t, uint8_t> birthRate = std::make_pair(10, 50);
    bool birthRateReduced = false;
    bool paused = false;
    float mutationFactor;

    UserActionType currUserAction;

    uint64_t focusedSimObjectID = UINT64_MAX;

    std::unordered_map<uint64_t, std::shared_ptr<SimObject>> simObjects;
    std::vector<std::shared_ptr<Organism>> nextGenParents;
    static std::mt19937 mt;
    SDL_Rect simBounds;
    std::shared_ptr<QuadTree> quadTreePtr;
    bool quadTreeVisible = false;

    static constexpr float clickWidth = 8.0f;
    static constexpr float clickHeight = 8.0f;
    static constexpr float organismWidth = 8.0f;
    static constexpr float organismHeight = 8.0f;
    static constexpr float foodWidth = 6.0f;
    static constexpr float foodHeight = 6.0f;

    void handleTimers(float deltaTIme);

    void createNextGeneration();

    void addFire();
    void addFoodOnBorder(Vec2 direction);
    void addFood(uint16_t amount);
    void addOrganism(Organism* organismPtr);
    void removeOrganism(uint64_t id);
    void reproduceOrganisms(const std::shared_ptr<Organism>& organism1Ptr, const std::shared_ptr<Organism>& organism2Ptr);
    void mutateOrganisms();
    void handleCollision(uint64_t id1, uint64_t id2);
    void updateSimBounds(const SDL_Rect& newSimBounds);
    void checkBounds(const std::shared_ptr<SimObject>& objectPtr) const;
    bool shouldMutate() const;
    void setMutationFactor(float newMutationFactor) {
        if(newMutationFactor >= 0.0f && newMutationFactor <= 1.0f)
            this->mutationFactor = newMutationFactor;
    }

    void handleAddFood();

    std::array<std::function<void (const UIData&)>, static_cast<size_t>(UserActionType::SIZE)> userActionFuncMapping = {
        [] (const UIData& uiData) {}, //none
        [this] (const UIData& uiData) {handleAddFood();}, //add_food
        [this] (const UIData& uiData) {paused = true;}, //pause
        [this] (const UIData& uiData) { //unpause
            paused = false;
            setUserAction(UserActionType::NONE, uiData);
        },
        [this] (const UIData& uiData) { //focus
            const uint64_t* simObjectIDPtr = std::get_if<SimObjectID>(&uiData);
            if(!simObjectIDPtr || !simObjects.contains(*simObjectIDPtr)) return;
            if(focusedSimObjectID != UINT64_MAX && simObjects.contains(focusedSimObjectID)) {
                simObjects[focusedSimObjectID]->setColor({0, 0, 0, 255});
            }
            focusedSimObjectID = *simObjectIDPtr;
            simObjects[*simObjectIDPtr]->setColor({255,192, 203, 255});
            setUserAction(UserActionType::NONE, uiData);
        },
        [this] (const UIData& uiData) { //unfocus
            setUserAction(UserActionType::NONE, uiData);
            if(focusedSimObjectID == UINT64_MAX) {
                return;
            }else if(!simObjects.contains(focusedSimObjectID)) {
                focusedSimObjectID = UINT64_MAX;
                return;
            }
            simObjects[focusedSimObjectID]->setColor({0,0, 0, 255});
            focusedSimObjectID = UINT64_MAX;
        },
    };

    void markForDeletion(const uint64_t id) {simObjects[id]->markForDeletion();}
    std::shared_ptr<SimObject> get(const uint64_t id) {
        if(!simObjects.contains(id)) return nullptr;
        return simObjects[id];
    }
    std::function<void (uint64_t id)> markForDeletionLambda = [this] (const uint64_t id) {this->markForDeletion(id);};
    std::function<std::shared_ptr<SimObject> (uint64_t id)> getLambda = [this] (const uint64_t id) {return this->get(id);};
    std::function<void()> currUserActionFunc = [this] () {userActionFuncMapping[0](UIData());};
    Vec2 getRandomPoint() const;
    static uint64_t getRandomID();
    static OrganismData getOrganismData(const std::shared_ptr<Organism>& organismPtr);
};

#endif //SIMULATION_HPP
