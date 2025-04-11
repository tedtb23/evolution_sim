#include "Simulation.hpp"
#include "SimStructs.hpp"
#include "QuadTree.hpp"
#include "UIStructs.hpp"
#include "Organism.hpp"
#include "StaticSimObjects.hpp"
#include "SDL3/SDL.h"
#include <sstream>
#include <iomanip>

std::mt19937 Simulation::mt{std::random_device{}()};

Simulation::Simulation(const SDL_Rect& simBounds, const uint16_t maxPopulation, const int genomeSize, const float initialMutationFactor = 0.25f) :
    simBounds(simBounds), maxPopulation(maxPopulation),
    mutationFactor((initialMutationFactor >= 0.0f && initialMutationFactor <= 1.0f) ? initialMutationFactor : 0.25f)
{
    quadTreePtr = std::make_shared<QuadTree>(SDL_FRect{
        static_cast<float>(simBounds.x),
        static_cast<float>(simBounds.y),
        static_cast<float>(simBounds.w),
        static_cast<float>(simBounds.h)}, 10);
    currUserAction = UserActionType::NONE;

    static SDL_Color color = {50, 0, 240, 255};

    for (int i = 0; i < maxPopulation; i++) {
        const uint64_t id = getRandomID();
        const Vec2 initialPosition = getRandomPoint();
        const SDL_FRect boundingBox{
            initialPosition.x, initialPosition.y, organismWidth, organismHeight
        };
        Organism organism(
                id,
                genomeSize,
                color,
                boundingBox,
                SimState(
                        &markForDeletionLambda,
                        &getLambda, quadTreePtr));
        addOrganism(&organism);
        color.r += 10;
        color.b += 15;
    }
    //for(int i = 0; i < 5; i++) {
        //addFood(500);
    //}
    addFoodOnBorder(Vec2(1.0f, 0.0f));
}

Vec2 Simulation::getRandomPoint() const {
    std::uniform_int_distribution<int> distX(simBounds.x, simBounds.x + simBounds.w);
    std::uniform_int_distribution<int> distY(simBounds.y, simBounds.y + simBounds.h);
    return {static_cast<float>(distX(mt)), static_cast<float>(distY(mt))};
}

void Simulation::addFire() {
    std::uniform_int_distribution<int> distX(simBounds.x, (simBounds.x + simBounds.w) - 100);
    std::uniform_int_distribution<int> distY(simBounds.y, (simBounds.y + simBounds.h) - 100);

    const uint64_t id = getRandomID();
    const SDL_FRect boundingBox{static_cast<float>(distX(mt)), static_cast<float>(distY(mt)), 100, 100};
    SDL_Color color{252, 119, 3, 255};
    Fire fire(
            id,
            boundingBox,
            color,
            SimState(
                    &markForDeletionLambda,
                    &getLambda, quadTreePtr));
    quadTreePtr->insert(QuadTree::QuadTreeObject(id, boundingBox));
    simObjects.emplace(id, std::make_shared<Fire>(std::move(fire)));
}

void Simulation::addFoodOnBorder(Vec2 direction) {
    int amount = 500;
    if(foodAmount + amount > maxPopulation) return;
    foodAmount += amount;

    std::uniform_int_distribution<int> distX;
    std::uniform_int_distribution<int> distY;

    if(direction == Vec2(1.0f, 0.0f)) {
        distX.param(std::uniform_int_distribution<int>::param_type((simBounds.x + simBounds.w) - 100, (simBounds.x + simBounds.w) - (int)foodWidth));
        distY.param(std::uniform_int_distribution<int>::param_type(simBounds.y, (simBounds.y + simBounds.h) - (int)foodHeight));
    }else if(direction == Vec2(0.0f, -1.0f)) {
        distX.param(std::uniform_int_distribution<int>::param_type(simBounds.x, (simBounds.x + simBounds.w) - (int)foodWidth));
        distY.param(std::uniform_int_distribution<int>::param_type(simBounds.y, simBounds.y + 100));
    }else if(direction == Vec2(-1.0f, 0.0f)) {
        distX.param(std::uniform_int_distribution<int>::param_type(simBounds.x, simBounds.x + 100));
        distY.param(std::uniform_int_distribution<int>::param_type(simBounds.y, (simBounds.y + simBounds.h) - (int)foodHeight));
    }else if(direction == Vec2(0.0f, 1.0f)) {
        distX.param(std::uniform_int_distribution<int>::param_type(simBounds.x, (simBounds.x + simBounds.w) - (int)foodWidth));
        distY.param(std::uniform_int_distribution<int>::param_type((simBounds.y + simBounds.h) - 100, (simBounds.y + simBounds.h) - (int)foodHeight));
    }else return;

    for(int i = 0; i < amount; i++) {
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
                        &markForDeletionLambda,
                        &getLambda, quadTreePtr));
        quadTreePtr->insert(QuadTree::QuadTreeObject(foodID, foodBoundingBox));
        simObjects.emplace(foodID, std::make_shared<Food>(std::move(food)));
    }
}

void Simulation::addFood(uint16_t amount) {
    if(foodAmount + amount > maxPopulation) return;
    foodAmount += amount;

    std::uniform_int_distribution<int> distX;
    std::uniform_int_distribution<int> distY;
    if(simBounds.w > 100 && simBounds.h > 100) {
        std::uniform_int_distribution<int> distInitialX(simBounds.x + 100, simBounds.x + simBounds.w - 100);
        std::uniform_int_distribution<int> distInitialY(simBounds.y + 100, simBounds.y + simBounds.h - 100);
        int initialX = distInitialX(mt);
        int initialY = distInitialY(mt);
        distX.param(std::uniform_int_distribution<int>::param_type(initialX - 100, initialX + 100));
        distY.param(std::uniform_int_distribution<int>::param_type(initialY - 100, initialY + 100));
    }else {
        distX.param(std::uniform_int_distribution<int>::param_type(simBounds.x, simBounds.x + simBounds.w - (int)foodWidth));
        distY.param(std::uniform_int_distribution<int>::param_type(simBounds.y, simBounds.y + simBounds.h - (int)foodHeight));
    }

    for(int i = 0; i < amount; i++) {
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
                        &markForDeletionLambda,
                        &getLambda, quadTreePtr));
        quadTreePtr->insert(QuadTree::QuadTreeObject(foodID, foodBoundingBox));
        simObjects.emplace(foodID, std::make_shared<Food>(std::move(food)));
    }
}

uint64_t Simulation::getRandomID() {
    static std::uniform_int_distribution<uint64_t> distID(0, UINT64_MAX - 1);
    return distID(mt);
}
bool Simulation::shouldMutate() const {
    static std::bernoulli_distribution distBool(mutationFactor);
    return distBool(mt);
}

void Simulation::render(SDL_Renderer *renderer) {
    //SDL_SetRenderDrawColor(renderer, 255, 0, 255, 255);
    //SDL_FRect r{200.0f, 0.0f, 30.0f, 30.0f};
    //SDL_RenderFillRect(renderer, &r);
    for(const auto & [id, object]: simObjects) {
       object->render(renderer);
    }
    if(quadTreeVisible) quadTreePtr->show(*renderer);
}

void Simulation::update(const SDL_Rect& newSimBounds, const float deltaTime) {
    updateSimBounds(newSimBounds);
    currUserActionFunc();

    if(paused) return;

    handleTimers(deltaTime);

    for(auto itr = simObjects.begin(); itr != simObjects.end();) {
        const uint64_t id = itr->first;
        const std::shared_ptr<SimObject> &objectPtr = itr->second;
        const std::shared_ptr<Organism> organismPtr = std::dynamic_pointer_cast<Organism>(objectPtr);

        objectPtr->update(deltaTime);
        if (organismPtr) {
            organismPtr->clearCollisionIDs();
            if (
                organismPtr->shouldReproduce() &&
                std::find(nextGenParents.begin(), nextGenParents.end(), organismPtr) == nextGenParents.end()
            )
                nextGenParents.emplace_back(organismPtr);
        }
        checkBounds(objectPtr);

        if (objectPtr->shouldDelete()) {
            quadTreePtr->remove(QuadTree::QuadTreeObject(id, objectPtr->getBoundingBox()));
            if (organismPtr) population--;
            if (std::dynamic_pointer_cast<Food>(objectPtr)) foodAmount--;
            itr = simObjects.erase(itr);
        } else {
            ++itr;
        }
    }
    for(const auto& [id1, id2] : quadTreePtr->getIntersections()) {
        handleCollision(id1, id2);
    }
}

void Simulation::fixedUpdate() {
    if(paused) return;

    quadTreePtr->undivide();
    for(auto & [id, objectPtr]: simObjects) {
        objectPtr->fixedUpdate();
        const std::shared_ptr<Organism> organismPtr = std::dynamic_pointer_cast<Organism>(objectPtr);
        if(organismPtr) {
            organismPtr->clearNeighbors();
            organismPtr->clearRaycastNeighbors();
            std::vector<std::pair<uint64_t, Vec2>> neighbors = quadTreePtr->getNearestNeighbors(
                    QuadTree::QuadTreeObject(
                            organismPtr->getID(),
                            organismPtr->getBoundingBox()));
            if(neighbors.empty()) {
                std::vector<std::pair<uint64_t, Vec2>> raycastNeighbors = quadTreePtr->raycast(
                        QuadTree::QuadTreeObject(
                                organismPtr->getID(),
                                organismPtr->getBoundingBox()));
                organismPtr->addRaycastNeighbors(raycastNeighbors);
            }else {
                organismPtr->addNeighbors(neighbors);
            }
        }
    }
}

void Simulation::handleTimers(const float deltaTime) {
    static int fireAmount = 0;
    if(foodTimer >= 15.0f) {
        //addFood(500);
        addFoodOnBorder(Vec2(1.0f, 0.0f));
        if(fireAmount < 10) addFire();
        fireAmount++;
        foodTimer = 0.0f;
    }else foodTimer += deltaTime;
    if(genTimer >= 10.0f) {
        createNextGeneration();
        genTimer = 0.0f;
    }else genTimer += deltaTime;
    if(mutationTimer >= 5.0f) {
            mutateOrganisms();
        mutationTimer = 0.0f;
    }else mutationTimer += deltaTime;
    if(birthRateTimer >= 180.0f && !birthRateReduced) {
        birthRate = std::make_pair(8, 10);
        birthRateReduced = true;
    }else birthRateTimer += deltaTime;
}

void Simulation::mutateOrganisms() {
    for(const auto& [id, objectPtr] : simObjects) {
        const std::shared_ptr organismPtr = std::dynamic_pointer_cast<Organism>(objectPtr);
        if(organismPtr)
            if(shouldMutate())
                organismPtr->mutateGenome();
    }
}

void Simulation::addOrganism(Organism* organismPtr) {
    quadTreePtr->insert(QuadTree::QuadTreeObject(organismPtr->getID(), organismPtr->getBoundingBox()));
    simObjects.emplace(organismPtr->getID(), std::make_shared<Organism>(std::move(*organismPtr)));
    population++;
}

//todo: implement me
void Simulation::removeOrganism(uint64_t id) {

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

void Simulation::reproduceOrganisms(const std::shared_ptr<Organism>& organism1Ptr, const std::shared_ptr<Organism>& organism2Ptr) {
    static SDL_Color color = {50, 0, 240, 255};
    if(population + birthRate.second >= maxPopulation) return;
    if(!organism1Ptr->shouldReproduce() || !organism2Ptr->shouldReproduce()) return;

    std::uniform_int_distribution<uint8_t> distNumChildren(birthRate.first, birthRate.second);
    const uint8_t numChildren = distNumChildren(mt);

    organism1Ptr->reproduce();
    organism2Ptr->reproduce();
    for(int i = 0; i < numChildren; i++) {
        const uint64_t newID = getRandomID();
        const Vec2 initialPosition = getRandomPoint();
        Organism newOrganism(
                newID,
                *organism1Ptr,
                *organism2Ptr,
                color,
                {initialPosition.x, initialPosition.y, organismWidth, organismHeight},
                SimState(
                        &markForDeletionLambda,
                        &getLambda, quadTreePtr));
        addOrganism(&newOrganism);
        color.r += 10;
        color.b += 15;
    }
}

void Simulation::updateSimBounds(const SDL_Rect& newSimBounds) {
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
}

void Simulation::checkBounds(const std::shared_ptr<SimObject>& objectPtr) const{
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


void Simulation::handleCollision(const uint64_t id1, const uint64_t id2) {
    std::shared_ptr<SimObject> object1Ptr(simObjects[id1]);
    std::shared_ptr<SimObject> object2Ptr(simObjects[id2]);
    std::shared_ptr<Organism> organism1Ptr = std::dynamic_pointer_cast<Organism>(object1Ptr);
    std::shared_ptr<Organism> organism2Ptr = std::dynamic_pointer_cast<Organism>(object2Ptr);
    std::shared_ptr<Fire> fire1Ptr = std::dynamic_pointer_cast<Fire>(object1Ptr);
    std::shared_ptr<Fire> fire2Ptr = std::dynamic_pointer_cast<Fire>(object2Ptr);

    //if(organism1Ptr && organism2Ptr) {
    //    const Vec2& organism1Velocity = organism1Ptr->getVelocity();
    //    const Vec2& organism2Velocity = organism2Ptr->getVelocity();
    //    organism1Ptr->setVelocity({organism2Velocity.x * 0.50f, organism2Velocity.y * 0.50f});
    //    organism2Ptr->setVelocity({organism1Velocity.x * 0.50f, organism1Velocity.y * 0.50f});
    //    //SDL_FRect organism1BoundingBox = organism1Ptr->getBoundingBox();
    //    //SDL_FRect organism2BoundingBox = organism2Ptr->getBoundingBox();
    //}

    if(fire1Ptr && !fire2Ptr) object2Ptr->markForDeletion();
    if(fire2Ptr && !fire1Ptr) object1Ptr->markForDeletion();

    if(organism1Ptr) {
        organism1Ptr->addCollisionID(id2);
        const Vec2& organism1Velocity = organism1Ptr->getVelocity();
        organism1Ptr->setVelocity({organism1Velocity.x * 0.95f, organism1Velocity.y * 0.95f});
    }
    if(organism2Ptr) {
        organism2Ptr->addCollisionID(id1);
        const Vec2& organism2Velocity = organism2Ptr->getVelocity();
        organism2Ptr->setVelocity({organism2Velocity.x * 0.95f, organism2Velocity.y * 0.95f});
    }
}

SimObjectData Simulation::userClicked(const float mouseX, const float mouseY) {
    SimObjectData result{};

    std::vector<uint64_t> objectsClicked = quadTreePtr->query(
        QuadTree::QuadTreeObject(
            SDL_FRect{mouseX, mouseY, clickWidth, clickHeight}));
    if(!objectsClicked.empty()) {
        std::shared_ptr<SimObject> objectPtr = simObjects[objectsClicked[0]];
        std::shared_ptr<Organism> organismPtr = std::dynamic_pointer_cast<Organism>(objectPtr);
        if(organismPtr) {
            result = getOrganismData(organismPtr);
        }
    }
    return result;
}

SimObjectData Simulation::getFocusedSimObjectData() {
    if(focusedSimObjectID == UINT64_MAX || !simObjects.contains(focusedSimObjectID)) return {};

    const std::shared_ptr<SimObject> objectPtr = simObjects[focusedSimObjectID];
    const std::shared_ptr<Organism> organismPtr = std::dynamic_pointer_cast<Organism>(objectPtr);

    if(organismPtr) return getOrganismData(organismPtr);
    return {}; //todo add more get methods for other types
}

OrganismData Simulation::getOrganismData(const std::shared_ptr<Organism>& organismPtr) {
    std::stringstream organismInfoStream;
    std::stringstream neuralNetInputStream;
    std::stringstream neuralNetOutputStream;
    std::vector<std::pair<NeuronInputType, float>> inputActivations = organismPtr->getInputActivations();
    std::vector<std::pair<NeuronOutputType, float>> outputActivations = organismPtr->getOutputActivations();

    organismInfoStream << "ID: " << organismPtr->getID() << std::endl <<
        "Hunger: " << static_cast<int>(organismPtr->getHunger()) << std::endl <<
        "Age: " << static_cast<int>(organismPtr->getAge());

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

    return {
            organismPtr->getID(),
            organismPtr->getHunger(), organismPtr->getAge(),
            inputActivations,
            outputActivations,
            organismInfoStream.str(),
            neuralNetInputStream.str(),
            neuralNetOutputStream.str()};
}

void Simulation::setUserAction(const UserActionType &userActionType, const UIData &uiData) {
    currUserAction = userActionType;
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
                        &markForDeletionLambda,
                        &getLambda, quadTreePtr));
        quadTreePtr->insert(QuadTree::QuadTreeObject(id, boundingBox));
        simObjects.emplace(id, std::make_shared<Food>(std::move(food)));
        clickedLastFrame = true;
    }
    if(!leftClicked) clickedLastFrame = false;
}
