#ifndef SIMSTATE_HPP
#define SIMSTATE_HPP

#include "QuadTree.hpp"
#include <functional>

class SimObject;

struct SimState {
    std::function<void (uint64_t otherID)>* markDeleteFuncPtr;
    std::function<std::shared_ptr<SimObject> (uint64_t id)>* getFuncPtr;
    std::shared_ptr<QuadTree> quadTreePtr;
    std::shared_ptr<SDL_Rect> simBoundsPtr;

    SimState(
    std::function<void (uint64_t otherID)>* markDeleteFuncPtr,
    std::function<std::shared_ptr<SimObject> (uint64_t id)>* getFuncPtr,
    const std::shared_ptr<QuadTree>& quadTreePtr,
    const std::shared_ptr<SDL_Rect>& initialSimBoundsPtr) :
        markDeleteFuncPtr(markDeleteFuncPtr),
        getFuncPtr(getFuncPtr),
        quadTreePtr(quadTreePtr),
        simBoundsPtr(initialSimBoundsPtr) {}
};
#endif //SIMSTATE_HPP
