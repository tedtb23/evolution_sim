#ifndef UISTRUCTS_HPP
#define UISTRUCTS_HPP

#include <vector>
#include <variant>
#include <cstdint>
#include "Neuron.hpp"

using SimObjectID = uint64_t;

using UIData = std::variant<SimObjectID>;


struct OrganismData {
    uint64_t id;
    uint8_t hunger;
    uint8_t age;
    std::string organismInfoStr;
    std::string neuralNetInputStr;
    std::string neuralNetOutputStr;
    std::string traitInfoStr;
};

using SimObjectData = std::variant<OrganismData>;

struct SimData {
    bool showPrimary;
    SimObjectData simObjectData;
    std::string quadTreeSizeStr;
    std::string populationStr;
    std::string generationStr;
};

enum class UserActionType {
    NONE,
    CHANGE_FOOD_RANGE,
    PAUSE,
    UNPAUSE,
    FOCUS,
    UNFOCUS,
    RANDOMIZE_SPAWN,
    SIZE
};

#endif //UISTRUCTS_HPP
