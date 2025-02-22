#ifndef NEURON_HPP
#define NEURON_HPP
#include <memory>
#include <optional>
#include <vector>
#include <variant>

struct Neuron;

enum class NeuronHiddenType {
    ZERO,
    ONE,
    TWO,
    THREE,
    FOUR,
    FIVE,
    SIX,
    SEVEN,
    EIGHT,
    NINE,
    SIZE
};

enum class NeuronInputType {
    SIGHT_OBJECT_FORWARD,
    SIZE
};

enum class NeuronOutputType {
    MOVE_LEFT,
    SIZE
};

struct NeuronConnection {
    std::shared_ptr<Neuron> neuronPtr;
    float weight;

    NeuronConnection(
        const std::shared_ptr<Neuron>& neuronPtr,
        const float weight)
    : neuronPtr(neuronPtr), weight(weight){};
};

struct Neuron {
    //std::variant<NeuronInputType, NeuronOutputType, NeuronHiddenType> type;
    float activation = INFINITY;
    float bias = NULL;
    std::optional<std::vector<NeuronConnection>> prevLayerConnections;
    std::optional<std::vector<NeuronConnection>> nextLayerConnections;

    explicit Neuron(const float bias) : bias(bias) {}
};

#endif //NEURON_HPP
