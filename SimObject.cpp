#include "SimObject.hpp"

void SimObject::render(SDL_Renderer* renderer) const {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    const SDL_FRect organismRect = {boundingBox.x, boundingBox.y, boundingBox.w, boundingBox.h};
    SDL_RenderFillRect(renderer, &organismRect);
}