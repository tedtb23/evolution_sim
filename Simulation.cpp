#include "Simulation.hpp"
#include "Neuron.hpp"
//#include "SimState.hpp"
#include "SDL3/SDL.h"

Simulation::Simulation(SDL_Window* window, const int initialOrganisms, const int genomeSize) {
    organisms.reserve(initialOrganisms);

    for(int i = 0; i < initialOrganisms; i++) {
        organisms.emplace_back(genomeSize, Vec2 {300.0f, 100.0f});
    }

    state.window = window;
}

void Simulation::render(SDL_Renderer *renderer) {
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    for(const Organism& organism : organisms) {
        const Vec2& position = organism.getPosition();
        const SDL_FRect organismRect = {position.x, position.y, Organism::width, Organism::height};
        SDL_RenderFillRect(renderer, &organismRect);
    }
}

void Simulation::update(const float deltaTime) {
    for(Organism& organism : organisms) {
        organism.update(state, deltaTime);
    }
}
