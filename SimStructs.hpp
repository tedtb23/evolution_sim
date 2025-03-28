#ifndef SIMSTATE_HPP
#define SIMSTATE_HPP

#include "QuadTree.hpp"

class SimObject;

struct SimState {
    std::function<void (const SimObject& simObject)>* addFuncPtr;
    std::function<void (uint64_t otherID)>* markDeleteFuncPtr;
    std::function<std::shared_ptr<SimObject> (uint64_t id)>* getFuncPtr;
    std::shared_ptr<QuadTree> quadTreePtr;

    SimState(
    std::function<void (const SimObject& simObject)>* addFuncPtr,
    std::function<void (uint64_t otherID)>* markDeleteFuncPtr,
    std::function<std::shared_ptr<SimObject> (uint64_t id)>* getFuncPtr,
        const std::shared_ptr<QuadTree>& quadTreePtr) :
        addFuncPtr(addFuncPtr),
        markDeleteFuncPtr(markDeleteFuncPtr),
        getFuncPtr(getFuncPtr),
        quadTreePtr(quadTreePtr) {}
};
#endif //SIMSTATE_HPP
