#ifndef UISTRUCTS_HPP
#define UISTRUCTS_HPP

#include <vector>
#include <variant>
#include <cstdint>
#include "Neuron.hpp"

struct UIData {
    //std::variant<>
};

struct OrganismData {
    float posX;
    float posY;
    uint64_t id;
    uint8_t hunger;
    uint8_t age;
    std::vector<std::pair<NeuronInputType, float>> inputActivations;
    //std::vector<std::pair<NeuronHiddenType, float>> hiddenActivations;
    std::vector<std::pair<NeuronOutputType, float>> outputActivations;
    std::string organismInfoStr;
    std::string neuralNetInputStr;
    std::string neuralNetOutputStr;
};

using SimData = std::variant<OrganismData>;

//struct SimData {
//    std::variant<OrganismData> data;
//};

enum class UserActionType {
    NONE,
    ADD_FOOD,
    PAUSE,
    UNPAUSE,
    SIZE
};

#endif //UISTRUCTS_HPP
