#ifndef SIMSTATE_HPP
#define SIMSTATE_HPP

#include "SDL3/SDL.h"
#include "QuadTree.hpp"

struct SimState {
    SDL_Rect simBounds;
    QuadTree quadTree;

    explicit SimState(const SDL_Rect& simBounds, const uint8_t quadTreeGranularity) :
        simBounds(simBounds),
        quadTree(
                SDL_FRect{
                    static_cast<float>(simBounds.x),
                    static_cast<float>(simBounds.y),
                    static_cast<float>(simBounds.w),
                    static_cast<float>(simBounds.h)}, quadTreeGranularity){}
};

#endif //SIMSTATE_HPP
