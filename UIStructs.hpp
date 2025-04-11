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
    std::vector<std::pair<NeuronInputType, float>> inputActivations;
    std::vector<std::pair<NeuronOutputType, float>> outputActivations;
    std::string organismInfoStr;
    std::string neuralNetInputStr;
    std::string neuralNetOutputStr;
};

using SimObjectData = std::variant<OrganismData>;

struct SimData {
    SimObjectData simObjectData;
    std::string quadTreeSizeStr;
    std::string populationStr;
    std::string generationStr;
};

enum class UserActionType {
    NONE,
    ADD_FOOD,
    PAUSE,
    UNPAUSE,
    FOCUS,
    UNFOCUS,
    SIZE
};

#endif //UISTRUCTS_HPP
