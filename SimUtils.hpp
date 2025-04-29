#ifndef SIMUTILS_HPP
#define SIMUTILS_HPP

#include "QuadTree.hpp"
#include <cmath>
#include <functional>
#include <random>

class SimObject;

namespace SimUtils {
    extern std::mt19937 mt;

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

    static inline SDL_FColor colorToFColor(const SDL_Color& color) {
        constexpr float range = 255.0f;

        return {
            .r = static_cast<float>(color.r) / range,
            .g = static_cast<float>(color.g) / range,
            .b = static_cast<float>(color.b) / range,
            .a = static_cast<float>(color.a) / range,
        };
    }
    static inline SDL_Rect fRecttoRect(const SDL_FRect& fRect) {
        return {
            static_cast<int>(fRect.x),
            static_cast<int>(fRect.y),
            static_cast<int>(fRect.w),
            static_cast<int>(fRect.h),
        };
    }
    static inline SDL_FRect rectToFRect(const SDL_Rect& rect) {
        return {
            static_cast<float>(rect.x),
            static_cast<float>(rect.y),
            static_cast<float>(rect.w),
            static_cast<float>(rect.h),
        };
    }
}
#endif //SIMUTILS_HPP
