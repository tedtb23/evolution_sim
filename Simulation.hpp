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

    SimData userClicked(float mouseX, float mouseY);
    UserActionType getCurrentUserAction() {return currUserAction;}
    void setUserAction(const UserActionType& userActionType, const UIData& uiData);

    int getCurrentPopulation() const {return population;}
    void showQuadTree(bool setQuadTreeVisible) {quadTreeVisible = setQuadTreeVisible;}
    [[nodiscard]] bool quadTreeIsShown() const {return quadTreeVisible;}
    [[nodiscard]] size_t getQuadSize() const {return quadTreePtr->size();}

private:
    uint16_t foodAmount = 0;
    uint16_t population = 0;
    const uint16_t maxPopulation;
    float foodTimer = 0.0f;
    float genTimer = 0.0f;
    bool paused = false;
    float mutationFactor;

    UserActionType currUserAction;

    std::unordered_map<uint64_t, std::shared_ptr<SimObject>> simObjects;
    std::vector<std::shared_ptr<Organism>> nextGenParents;
    static std::mt19937 mt;
    SDL_Rect simBounds;
    std::shared_ptr<QuadTree> quadTreePtr;
    bool quadTreeVisible = false;

    static constexpr float clickWidth = 6.0f;
    static constexpr float clickHeight = 6.0f;
    static constexpr float organismWidth = 6.0f;
    static constexpr float organismHeight = 6.0f;
    static constexpr float foodWidth = 6.0f;
    static constexpr float foodHeight = 6.0f;

    void handleTimers(float deltaTIme);

    void createNextGeneration();

    void addFood(uint16_t amount);
    void addOrganism(Organism* organismPtr);
    void removeOrganism(uint64_t id);
    void reproduceOrganisms(const std::shared_ptr<Organism>& organism1Ptr, const std::shared_ptr<Organism>& organism2Ptr);
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
        [] (const UIData& uiData) {},
        [this] (const UIData& uiData) {this->handleAddFood();},
        [this] (const UIData& uiData) {this->paused = true;},
        [this] (const UIData& uiData) {
            this->paused = false;
            setUserAction(UserActionType::NONE, uiData);
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
    static uint64_t getRandomID();
    static OrganismData getOrganismData(const std::shared_ptr<Organism>& organismPtr);
};

#endif //SIMULATION_HPP
