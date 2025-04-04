#include "Simulation.hpp"
#include "SimStructs.hpp"
#include "QuadTree.hpp"
#include "UIStructs.hpp"
#include "Organism.hpp"
#include "StaticSimObjects.hpp"
#include "SDL3/SDL.h"

std::mt19937 Simulation::mt{std::random_device{}()};

Simulation::Simulation(const SDL_Rect& simBounds, const int initialOrganisms, const int genomeSize, const float initialMutationFactor = 0.25f) :
    simBounds(simBounds), mutationFactor((initialMutationFactor >= 0.0f && initialMutationFactor <= 1.0f) ? initialMutationFactor : 0.25f)
{
    quadTreePtr = std::make_shared<QuadTree>(SDL_FRect{
        static_cast<float>(simBounds.x),
        static_cast<float>(simBounds.y),
        static_cast<float>(simBounds.w),
        static_cast<float>(simBounds.h)}, 10);

    std::uniform_int_distribution<int> distX(simBounds.x, simBounds.x + simBounds.w);
    std::uniform_int_distribution<int> distY(simBounds.y, simBounds.y + simBounds.h);

    for (int i = 0; i < initialOrganisms; i++) {
        const uint64_t id = getRandomID();
        const SDL_FRect boundingBox{
            static_cast<float>(distX(mt)), static_cast<float>(distY(mt)), organismWidth, organismHeight
        };
        Organism organism(
                id,
                genomeSize,
                boundingBox,
                SimState(
                        &addLambda,
                        &markForDeletionLambda,
                        &getLambda, quadTreePtr));
        quadTreePtr->insert(QuadTree::QuadTreeObject(id, boundingBox));
        simObjects.emplace(id, std::make_shared<Organism>(std::move(organism)));
    }
    for(int i = 0; i < 1000; i++) {
        const SDL_FRect foodBoundingBox{
            static_cast<float>(distX(mt)), static_cast<float>(distY(mt)), foodWidth, foodHeight
        };
        SDL_Color color{0, 255, 0, 255};
        const uint64_t foodID = getRandomID();
        Food food(
                foodID,
                foodBoundingBox,
                color,
                100,
                SimState(
                        &addLambda,
                        &markForDeletionLambda,
                        &getLambda, quadTreePtr));
        quadTreePtr->insert(QuadTree::QuadTreeObject(foodID, foodBoundingBox));
        simObjects.emplace(foodID, std::make_shared<Food>(std::move(food)));
    }
}

uint64_t Simulation::getRandomID() {
    static std::uniform_int_distribution<uint64_t> distID(0, UINT64_MAX);
    return distID(mt);
}
bool Simulation::shouldMutate() const {
    static std::bernoulli_distribution distBool(mutationFactor);
    return distBool(mt);
}

void Simulation::render(SDL_Renderer *renderer) {
    for(const auto & [id, object]: simObjects) {
       object->render(renderer);
    }
    if(quadTreeVisible) quadTreePtr->show(*renderer);
}

void Simulation::update(const SDL_Rect& newSimBounds, const float deltaTime) {
    if(simBounds.x != newSimBounds.x ||
       simBounds.y != newSimBounds.y ||
       simBounds.w != newSimBounds.w ||
       simBounds.h != newSimBounds.h) {

        simBounds = newSimBounds;
        quadTreePtr = std::make_shared<QuadTree>(SDL_FRect{
            static_cast<float>(simBounds.x),
            static_cast<float>(simBounds.y),
            static_cast<float>(simBounds.w),
            static_cast<float>(simBounds.h)}, 10);
        for(const auto& [id, objectPtr] : simObjects) {
            quadTreePtr->insert(QuadTree::QuadTreeObject(id, objectPtr->getBoundingBox()));
            objectPtr->newQuadTree(quadTreePtr);
        }
    }

    if(paused) return;

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
    if(paused) return;

    quadTreePtr->undivide();
    for(auto & [id, objectPtr]: simObjects) {
        objectPtr->fixedUpdate();
        const std::shared_ptr<Organism> organismPtr = std::dynamic_pointer_cast<Organism>(objectPtr);
        if(organismPtr)
            if(shouldMutate())
                organismPtr->mutateGenome();
    }
}

void Simulation::checkBounds(const std::shared_ptr<SimObject> &objectPtr) const{
    const auto leftBound = static_cast<float>(simBounds.x);
    const auto rightBound = static_cast<float>(simBounds.x + simBounds.w);
    const auto topBound = static_cast<float>(simBounds.y);
    const auto bottomBound = static_cast<float>(simBounds.y + simBounds.h);
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
        quadTreePtr->remove(QuadTree::QuadTreeObject(objectPtr->getID(), oldBoundingBox));
        quadTreePtr->insert(QuadTree::QuadTreeObject(objectPtr->getID(), boundingBox));
    }
    objectPtr->setBoundingBox(boundingBox);
}


void Simulation::handleCollisions() {
    for(const auto& [id, objectPtr] : simObjects) {
        checkBounds(objectPtr);
    }

    std::vector<std::pair<uint64_t, uint64_t>> collisions = quadTreePtr->getIntersections();

    for(const auto& [id1, id2] : collisions) {
        const std::shared_ptr<Organism> organism1Ptr = std::dynamic_pointer_cast<Organism>(simObjects[id1]);
        const std::shared_ptr<Organism> organism2Ptr = std::dynamic_pointer_cast<Organism>(simObjects[id2]);

        if(organism1Ptr) {
            organism1Ptr->addCollisionID(id2);
            organism1Ptr->setColor(SDL_Color {0, 0, 255, 255});
            const Vec2& velocity = organism1Ptr->getVelocity();
            organism1Ptr->setVelocity({velocity.x * 0.50f, velocity.y * 0.50f});
        }
        if(organism2Ptr) {
            organism2Ptr->addCollisionID(id1);
            organism2Ptr->setColor(SDL_Color {0, 0, 255, 255});
            const Vec2& velocity = organism2Ptr->getVelocity();
            organism2Ptr->setVelocity({velocity.x * 0.50f, velocity.y * 0.50f});
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
        Food food(
                id,
                boundingBox,
                color,
                100,
                SimState(
                        &addLambda,
                        &markForDeletionLambda,
                        &getLambda, quadTreePtr));
        quadTreePtr->insert(QuadTree::QuadTreeObject(id, boundingBox));
        simObjects.emplace(id, std::make_shared<Food>(std::move(food)));
        clickedLastFrame = true;
    }
    if(!leftClicked) clickedLastFrame = false;
}
