#ifndef SIMOBJECT_HPP
#define SIMOBJECT_HPP

#include "SDL3/SDL.h"
#include <cstdint>

struct SimObject {
    uint64_t id;
    SDL_FRect boundingBox;


    SimObject(const uint64_t id, const SDL_FRect& boundingBox) : id(id), boundingBox(boundingBox) {};
};

#endif //SIMOBJECT_HPP
