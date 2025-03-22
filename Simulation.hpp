#ifndef SIMULATION_HPP
#define SIMULATION_HPP

#include "SimStructs.hpp"
#include "SDL3/SDL.h"
#include "Organism.hpp"
#include <unordered_map>
#include <memory>

class Simulation {
public:
    Simulation(const SDL_Rect& simBounds, int initialOrganisms, int genomeSize);
    void update(const SDL_Rect& simBounds, float deltaTime);
    void fixedUpdate();
    void render(SDL_Renderer* renderer);
    void setGenomeSize(int genomeSize);


private:
    std::unordered_map<uint64_t, Organism> organisms;
    std::shared_ptr<SimState> statePtr;
};


#endif //SIMULATION_HPP
