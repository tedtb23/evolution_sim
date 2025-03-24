#include "Simulation.hpp"
#include "SimStructs.hpp"
#include "SimObject.hpp"
#include "Organism.hpp"
#include "SDL3/SDL.h"

Simulation::Simulation(const SDL_Rect& simBounds, const int initialOrganisms, const int genomeSize) :
    statePtr(std::make_shared<SimState>(simBounds, 10)) {
    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_int_distribution<uint64_t> distID(0, UINT64_MAX);
    std::uniform_int_distribution<int> distX(simBounds.x, simBounds.w);
    std::uniform_int_distribution<int> distY(simBounds.y, simBounds.h);

    for(int i = 0; i < initialOrganisms; i++) {
        const uint64_t id = distID(mt);
        const Vec2 initialPosition(static_cast<float>(distX(mt)), static_cast<float>(distY(mt)));
        Organism organism(id, statePtr, genomeSize, initialPosition);
        statePtr->quadTree.insert(SimObject(id, SDL_FRect{initialPosition.x, initialPosition.y, Organism::width, Organism::height}));
        organisms.emplace(id, std::move(organism));
    }
}

void Simulation::render(SDL_Renderer *renderer) {
    for(auto & [id, organism]: organisms) {
        SDL_Color color = organism.getColor();
        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
        const Vec2& position = organism.getPosition();
        const SDL_FRect organismRect = {position.x, position.y, Organism::width, Organism::height};
        SDL_RenderFillRect(renderer, &organismRect);
    }
    statePtr->quadTree.show(*renderer);
}

void Simulation::update(const SDL_Rect& simBounds, const float deltaTime) {
    if(statePtr->simBounds.x != simBounds.x ||
       statePtr->simBounds.y != simBounds.y ||
       statePtr->simBounds.w != simBounds.w ||
       statePtr->simBounds.h != simBounds.h) {

        statePtr->simBounds = simBounds;
        statePtr->quadTree = std::move(QuadTree(
                SDL_FRect{
                static_cast<float>(simBounds.x),
                static_cast<float>(simBounds.y),
                static_cast<float>(simBounds.w),
                static_cast<float>(simBounds.h)}, 10));
    }

    for(auto & [id, organism]: organisms) {
        organism.update(deltaTime);
    }
}

void Simulation::fixedUpdate() {
    statePtr->quadTree.undivide();
    for(auto & [id, organism]: organisms) {
        organism.fixedUpdate();
    }
}
