#ifndef SIMULATION_HPP
#define SIMULATION_HPP

#include "SimObject.hpp"
#include "Organism.hpp"
#include "SimStructs.hpp"
#include "UIStructs.hpp"
#include "SDL3/SDL.h"
#include <unordered_map>
#include <memory>

class Simulation{
public:
    Simulation(const SDL_Rect& simBounds, int initialOrganisms, int genomeSize);
    void update(const SDL_Rect& simBounds, float deltaTime);
    void fixedUpdate();
    void render(SDL_Renderer* renderer);
    void setGenomeSize(int genomeSize);

    void setUserAction(const UserActionType& userActionType, const UIData& uiData);

    void showQuadTree(bool quadTreeVisible) {this->quadTreeVisible = quadTreeVisible;}
    [[nodiscard]] size_t getQuadSize() const {return quadTreePtr->size();}

private:
    //maybe don't use inheritance here
    std::unordered_map<uint64_t, std::shared_ptr<SimObject>> simObjects;
    SDL_Rect simBounds;
    std::shared_ptr<QuadTree> quadTreePtr;
    bool quadTreeVisible = false;

    static constexpr float organismWidth = 6.0f;
    static constexpr float organismHeight = 6.0f;
    static constexpr float foodWidth = 6.0f;
    static constexpr float foodHeight = 6.0f;

    void handleCollisions();
    void checkBounds(const std::shared_ptr<SimObject>& objectPtr);

    void handleAddFood();

    std::array<std::function<void (const UIData&)>, static_cast<size_t>(UserActionType::SIZE)> userActionFuncMapping = {
         [] (const UIData& uiData) {},
         [this] (const UIData& uiData) {this->handleAddFood();},
    };
    void add(const SimObject& simObject) {simObjects.emplace(simObject.getID(), std::make_unique<SimObject>(simObject));}
    void markForDeletion(const uint64_t id) {simObjects[id]->markForDeletion();}
    std::shared_ptr<SimObject> get(const uint64_t id) {return simObjects[id];}
    std::function<void (uint64_t id)> markForDeletionLambda = [this] (const uint64_t id) {this->markForDeletion(id);};
    std::function<void (const SimObject& simObject)> addLambda = [this] (const SimObject& simObject) {this->add(simObject);};
    std::function<std::shared_ptr<SimObject> (uint64_t id)> getLambda = [this] (const uint64_t id) {return this->get(id);};

    static uint64_t getRandomID();
    std::function<void()> currUserActionFunc = [this] () {userActionFuncMapping[0];};
};

#endif //SIMULATION_HPP
