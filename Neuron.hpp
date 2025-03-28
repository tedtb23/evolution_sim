#ifndef NEURON_HPP
#define NEURON_HPP
#include <memory>
#include <optional>
#include <vector>

#define NEURONHIDDENTYPE_VALUES(...) constexpr NeuronHiddenType hiddenValues[] = {__VA_ARGS__}
#define NEURONHIDDENTYPE_SIZE (sizeof(hiddenValues) / sizeof(hiddenValues[0]))
#define NEURONINPUTTYPE_VALUES(...) constexpr NeuronInputType inputValues[] = {__VA_ARGS__}
#define NEURONINPUTTYPE_SIZE (sizeof(inputValues) / sizeof(inputValues[0]))
#define NEURONOUTPUTTYPE_VALUES(...) constexpr NeuronOutputType outputValues[] = {__VA_ARGS__}
#define NEURONOUTPUTTYPE_SIZE (sizeof(outputValues) / sizeof(outputValues[0]))

struct Neuron;

enum NeuronHiddenType {
    ZERO,
    ONE,
    TWO,
    THREE,
    FOUR,
    FIVE,
    SIX,
    SEVEN,
    EIGHT,
    NINE
};
NEURONHIDDENTYPE_VALUES(ZERO,ONE,TWO,THREE,FOUR,FIVE,SIX,SEVEN,EIGHT,NINE);

enum NeuronInputType {
    SIGHT_OBJECT_FORWARD,
};
NEURONINPUTTYPE_VALUES(SIGHT_OBJECT_FORWARD);

enum NeuronOutputType {
    MOVE_LEFT = SIGHT_OBJECT_FORWARD + 1,
    MOVE_RIGHT,
    MOVE_UP,
    MOVE_DOWN,
    EAT,

};
NEURONOUTPUTTYPE_VALUES(MOVE_LEFT, MOVE_RIGHT, MOVE_UP, MOVE_DOWN, EAT, );

struct NeuronConnection {
    std::shared_ptr<Neuron> neuronPtr;
    float weight;

    NeuronConnection(
        const std::shared_ptr<Neuron>& neuronPtr,
        const float weight)
    : neuronPtr(neuronPtr), weight(weight){};
};

struct Neuron {
    float activation = 0.0f;
    float bias;
    std::optional<std::vector<NeuronConnection>> prevLayerConnections;

    explicit Neuron(const float bias) : bias(bias) {}
};

#endif //NEURON_HPP
