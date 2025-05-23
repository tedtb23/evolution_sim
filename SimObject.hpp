#ifndef SIMOBJECT_HPP
#define SIMOBJECT_HPP

#include "SimUtils.hpp"
#include "QuadTree.hpp"
#include "SDL3/SDL.h"
#include <cstdint>
#include <memory>
#include <utility>

class SimObject {
public:
    SimObject(const uint64_t id, const SDL_FRect& boundingBox, SimUtils::SimState simState, const bool inQuadTree) :
    id(id),
    color({0, 0, 0, 255}),
    boundingBox(boundingBox),
    simState(std::move(simState)),
    inQuadTree(inQuadTree) {}
    SimObject(const uint64_t id, const SDL_FRect& boundingBox, const SDL_Color& initialColor, SimUtils::SimState simState, const bool inQuadTree) :
    id(id),
    boundingBox(boundingBox),
    color(initialColor),
    simState(std::move(simState)),
    inQuadTree(inQuadTree) {}
    virtual ~SimObject() = default;

    [[nodiscard]] uint64_t getID() const {return id;}
    [[nodiscard]] SDL_FRect getBoundingBox() const {return boundingBox;}
    [[nodiscard]] virtual Vec2 getPosition() const {return {boundingBox.x, boundingBox.y};}
    void setBoundingBox(const SDL_FRect& newBoundingBox) {boundingBox = newBoundingBox;}
    [[nodiscard]] SDL_Color getColor() const {return color;}
    void setColor(const SDL_Color& newColor) {color = newColor;}
    void newQuadTree(const std::shared_ptr<QuadTree>& quadTreePtr) {this->simState.quadTreePtr = quadTreePtr;}

    void markForDeletion() {markedForDeletion = true;}
    [[nodiscard]] bool shouldDelete() const {return markedForDeletion;}
    [[nodiscard]] bool isInQuadTree() const {return inQuadTree;}

    virtual void update(const float deltaTime) {}
    virtual void fixedUpdate() {}
    virtual void render(SDL_Renderer* rendererPtr) const;

protected:
    SimUtils::SimState simState;
    uint64_t id;
    SDL_FRect boundingBox;
    SDL_Color color;
    bool markedForDeletion = false;
    bool inQuadTree;
};



#endif //SIMOBJECT_HPP
