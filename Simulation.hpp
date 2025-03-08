#ifndef SIMULATION_HPP
#define SIMULATION_HPP

#include "SimState.hpp"
#include "Organism.hpp"
#include "SDL3/SDL.h"
#include <vector>

class Simulation {
public:
    Simulation(SDL_Window* window, int initialOrganisms, int genomeSize);
    void update(float deltaTime);
    void render(SDL_Renderer* renderer);
    void setGenomeSize(int genomeSize);


private:
    std::vector<Organism> organisms;
    SimState state{};
};


#endif //SIMULATION_HPP
