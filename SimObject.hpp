#ifndef SIMOBJECT_HPP
#define SIMOBJECT_HPP

#include "SDL3/SDL.h"
#include <cstdint>

struct SimObject {
    uint64_t id;
    SDL_FRect boundingBox;


    SimObject(const uint64_t id, const SDL_FRect& boundingBox) : id(id), boundingBox(boundingBox) {};

    bool operator==(const SimObject& other) const {
        return this->id == other.id;
    }
    bool operator<(const SimObject& other) const {
        return this->id < other.id;
    }
    bool operator>(const SimObject& other) const {
        return this->id > other.id;
    }
};

struct SimObjectPair {
    SimObject first;
    SimObject second;

    SimObjectPair(const SimObject& first, const SimObject& second) : first(first), second(second) {};

    bool operator==(const SimObjectPair& other) const {
        const auto& thisSmaller = std::min(this->first, this->second);
        const auto& thisLarger = std::max(this->first, this->second);
        const auto& otherSmaller = std::min(other.first, other.second);
        const auto& otherLarger = std::max(other.first, other.second);
        return thisSmaller == otherSmaller && thisLarger == otherLarger;
    }
};

struct SimObjectPairHash {
    std::size_t operator()(const SimObjectPair& pair) const {
        return std::hash<uint64_t>()(pair.first.id) ^ std::hash<uint64_t>()(pair.second.id);
    }
};

#endif //SIMOBJECT_HPP
