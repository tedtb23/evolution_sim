#include "Simulation.hpp"
#include "SimStructs.hpp"
#include "QuadTree.hpp"
#include "UIStructs.hpp"
#include "Organism.hpp"
#include "StaticSimObjects.hpp"
#include "SDL3/SDL.h"

Simulation::Simulation(const SDL_Rect& simBounds, const int initialOrganisms, const int genomeSize) :
    simBounds(simBounds)
{
    quadTreePtr = std::make_shared<QuadTree>(SDL_FRect{
        static_cast<float>(simBounds.x),
        static_cast<float>(simBounds.y),
        static_cast<float>(simBounds.w),
        static_cast<float>(simBounds.h)}, 10);

    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_int_distribution<int> distX(simBounds.x, simBounds.x + simBounds.w);
    std::uniform_int_distribution<int> distY(simBounds.y, simBounds.y + simBounds.h);

    for (int i = 0; i < initialOrganisms; i++) {
        const uint64_t id = getRandomID();
        const SDL_FRect boundingBox{
            static_cast<float>(distX(mt)), static_cast<float>(distY(mt)), organismWidth, organismHeight
        };
        Organism organism(id, genomeSize, boundingBox, SimState(&addLambda, &markForDeletionLambda, &getLambda, quadTreePtr));
        quadTreePtr->insert(QuadTree::QuadTreeObject(id, boundingBox));
        simObjects.emplace(id, std::make_shared<Organism>(std::move(organism)));
    }
    for(int i = 0; i < 1000; i++) {
        const SDL_FRect foodBoundingBox{
            static_cast<float>(distX(mt)), static_cast<float>(distY(mt)), foodWidth, foodHeight
        };
        SDL_Color color{0, 255, 0, 255};
        const uint64_t foodID = getRandomID();
        Food food(foodID, foodBoundingBox, color, 100, SimState(&addLambda, &markForDeletionLambda, &getLambda, quadTreePtr));
        quadTreePtr->insert(QuadTree::QuadTreeObject(foodID, foodBoundingBox));
        simObjects.emplace(foodID, std::make_shared<Food>(std::move(food)));
    }
}

uint64_t Simulation::getRandomID() {
    static std::random_device rd;
    static std::mt19937 mt(rd());
    static std::uniform_int_distribution<uint64_t> distID(0, UINT64_MAX);
    return distID(mt);
}

void Simulation::render(SDL_Renderer *renderer) {
    for(const auto & [id, object]: simObjects) {
       object->render(renderer);
    }
    if(quadTreeVisible) quadTreePtr->show(*renderer);
}

void Simulation::update(const SDL_Rect& simBounds, const float deltaTime) {
    if(this->simBounds.x != simBounds.x ||
       this->simBounds.y != simBounds.y ||
       this->simBounds.w != simBounds.w ||
       this->simBounds.h != simBounds.h) {

        this->simBounds = simBounds;
        quadTreePtr = std::make_shared<QuadTree>(SDL_FRect{
            static_cast<float>(simBounds.x),
            static_cast<float>(simBounds.y),
            static_cast<float>(simBounds.w),
            static_cast<float>(simBounds.h)}, 10);
        for(const auto& [id, object] : simObjects) {
            quadTreePtr->insert(QuadTree::QuadTreeObject(id, object->getBoundingBox()));
            object->newQuadTree(quadTreePtr);
        }
    }
    currUserActionFunc();

    for(auto itr = simObjects.begin(); itr != simObjects.end();) {
        const uint64_t id = itr->first;
        const std::shared_ptr<SimObject>& objectPtr = itr->second;

        objectPtr->update(deltaTime);

        if(objectPtr->shouldDelete()) {
            quadTreePtr->remove(QuadTree::QuadTreeObject(id, objectPtr->getBoundingBox()));
            itr = simObjects.erase(itr);
        }else {
            ++itr;
        }
    }
    handleCollisions();
}

void Simulation::fixedUpdate() {
    quadTreePtr->undivide();
    for(auto & [id, object]: simObjects) {
        object->fixedUpdate();
    }
}

void Simulation::checkBounds(const std::shared_ptr<SimObject> &objectPtr) {
    const auto leftBound = static_cast<float>(simBounds.x);
    const auto rightBound = static_cast<float>(simBounds.x + simBounds.w);
    const auto topBound = static_cast<float>(simBounds.y);
    const auto bottomBound = static_cast<float>(simBounds.y + simBounds.h);
    SDL_FRect boundingBox = objectPtr->getBoundingBox();

    if(boundingBox.x < leftBound)
        boundingBox.x = leftBound;
    if(boundingBox.x + boundingBox.w > rightBound)
        boundingBox.x = rightBound - boundingBox.w;
    if(boundingBox.y < topBound)
        boundingBox.y = topBound;
    if(boundingBox.y + boundingBox.h > bottomBound)
        boundingBox.y = bottomBound - boundingBox.h;

    objectPtr->setBoundingBox(boundingBox);
}


void Simulation::handleCollisions() {
    std::vector<std::pair<uint64_t, uint64_t>> collisions = quadTreePtr->getIntersections();

    for(const auto& [id, objectPtr] : simObjects) {
        checkBounds(objectPtr);
    }

    for(const auto& [id1, id2] : collisions) {
        std::string str = typeid(*simObjects[id2]).name();
        std::string str2 = typeid(*simObjects[id1]).name();
        if(typeid(Organism) == typeid(*simObjects[id1])) {
            simObjects[id1]->setColor(SDL_Color {0, 0, 255, 255});
            const std::shared_ptr<Organism> organismPtr = std::dynamic_pointer_cast<Organism>(simObjects[id1]);
            const Vec2& velocity = organismPtr->getVelocity();
            organismPtr->setVelocity({velocity.x * 0.50f, velocity.y * 0.50f});
        }
        if(typeid(Organism) == typeid(*simObjects[id2])) {
            simObjects[id2]->setColor(SDL_Color {0, 0, 255, 255});
            const std::shared_ptr<Organism> organismPtr = std::dynamic_pointer_cast<Organism>(simObjects[id2]);
            const Vec2& velocity = organismPtr->getVelocity();
            organismPtr->setVelocity({velocity.x * 0.50f, velocity.y * 0.50f});
        }
    }
}

void Simulation::setUserAction(const UserActionType &userActionType, const UIData &uiData) {
    currUserActionFunc = [this, userActionType, uiData] () {userActionFuncMapping[static_cast<size_t>(userActionType)](uiData);};
}


void Simulation::handleAddFood() {
    static bool clickedLastFrame = false;
    float floatX, floatY;
    const SDL_MouseButtonFlags mouseState = SDL_GetMouseState(&floatX, &floatY);
    const bool leftClicked = mouseState & SDL_BUTTON_LMASK;
    if(!leftClicked) clickedLastFrame = false;
    const int x = static_cast<int>(floatX);
    const int y = static_cast<int>(floatY);

    if(!clickedLastFrame && leftClicked &&
        x >= simBounds.x &&
        x < simBounds.x + simBounds.w &&
        y >= simBounds.y &&
        y < simBounds.y + simBounds.h) {
        const uint64_t id = getRandomID();
        const SDL_FRect boundingBox{floatX, floatY, foodWidth, foodHeight};
        SDL_Color color{0, 255, 0, 255};
        Food food(id, boundingBox, color, 100, SimState(&addLambda, &markForDeletionLambda, &getLambda, quadTreePtr));
        quadTreePtr->insert(QuadTree::QuadTreeObject(id, boundingBox));
        simObjects.emplace(id, std::make_shared<Food>(std::move(food)));
        clickedLastFrame = true;
    }
}
