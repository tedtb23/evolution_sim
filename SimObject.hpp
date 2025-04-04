#ifndef SIMOBJECT_HPP
#define SIMOBJECT_HPP

#include "SimStructs.hpp"
#include "QuadTree.hpp"
#include "SDL3/SDL.h"
#include <cstdint>
#include <memory>
#include <utility>

class SimObject {
public:
    SimObject(const uint64_t id, const SDL_FRect& boundingBox, SimState  simState) :
    id(id),
    boundingBox(boundingBox),
    simState(std::move(simState)) {}
    SimObject(const uint64_t id, const SDL_FRect& boundingBox, const SDL_Color& initialColor, SimState simState) :
    id(id),
    boundingBox(boundingBox),
    color(initialColor),
    simState(std::move(simState)) {}
    virtual ~SimObject() = default;


    [[nodiscard]] uint64_t getID() const {return id;}
    [[nodiscard]] SDL_FRect getBoundingBox() const {return boundingBox;}
    void setBoundingBox(const SDL_FRect& newBoundingBox) {boundingBox = newBoundingBox;}
    [[nodiscard]] SDL_Color getColor() const {return color;}
    void setColor(const SDL_Color& newColor) {color = newColor;}
    void newQuadTree(const std::shared_ptr<QuadTree>& quadTreePtr) {this->simState.quadTreePtr = quadTreePtr;}

    void markForDeletion() {markedForDeletion = true;}
    [[nodiscard]] bool shouldDelete() const {return markedForDeletion;}

    virtual void update(const float deltaTime) {}
    virtual void fixedUpdate() {}
    virtual void render(SDL_Renderer* renderer) const;

protected:
    SimState simState;
    uint64_t id;
    SDL_FRect boundingBox;
    SDL_Color color{};
    bool markedForDeletion = false;
};



#endif //SIMOBJECT_HPP
