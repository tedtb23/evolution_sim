#ifndef NEURON_HPP
#define NEURON_HPP
#include <memory>
#include <optional>
#include <vector>

#define NEURONHIDDENTYPE_VALUES(...) constexpr char const* hiddenValues[] = {__VA_ARGS__}
#define NEURONHIDDENTYPE_SIZE (sizeof(hiddenValues) / sizeof(hiddenValues[0]))
#define NEURONINPUTTYPE_VALUES(...) constexpr char const* inputValues[] = {__VA_ARGS__}
#define NEURONINPUTTYPE_SIZE (sizeof(inputValues) / sizeof(inputValues[0]))
#define NEURONOUTPUTTYPE_VALUES(...) constexpr char const* outputValues[] = {__VA_ARGS__}
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
NEURONHIDDENTYPE_VALUES("ZERO","ONE","TWO","THREE","FOUR","FIVE","SIX","SEVEN","EIGHT","NINE");

enum NeuronInputType {
    HUNGER,
    FOOD_LEFT,
    FOOD_RIGHT,
    FOOD_UP,
    FOOD_DOWN,
    FOOD_COLLISION,
    ORGANISM_LEFT,
    ORGANISM_RIGHT,
    ORGANISM_UP,
    ORGANISM_DOWN,
    ORGANISM_COLLISION,
    FIRE_LEFT,
    FIRE_RIGHT,
    FIRE_UP,
    FIRE_DOWN,
    DETECT_DANGER_PHEROMONE,
    BOUNDS_LEFT,
    BOUNDS_RIGHT,
    BOUNDS_UP,
    BOUNDS_DOWN,
    TEMPERATURE,
    OXYGEN_SATURATION,
    HYDROGEN_SATURATION,
};
NEURONINPUTTYPE_VALUES(
        "HUNGER",
        "FOOD_LEFT",
        "FOOD_RIGHT",
        "FOOD_UP",
        "FOOD_DOWN",
        "FOOD_COLLISION",
        "ORGANISM_LEFT",
        "ORGANISM_RIGHT",
        "ORGANISM_UP",
        "ORGANISM_DOWN",
        "ORGANISM_COLLISION",
        "FIRE_LEFT",
        "FIRE_RIGHT",
        "FIRE_UP",
        "FIRE_DOWN",
        "DETECT_DANGER_PHEROMONE",
        "BOUNDS_LEFT",
        "BOUNDS_RIGHT",
        "BOUNDS_UP",
        "BOUNDS_DOWN",
        "TEMPERATURE",
        "OXYGEN_SATURATION",
        "HYDROGEN_SATURATION",
);

enum NeuronOutputType {
    MOVE_LEFT = HYDROGEN_SATURATION + 1,
    MOVE_RIGHT,
    MOVE_UP,
    MOVE_DOWN,
    EAT,
};
NEURONOUTPUTTYPE_VALUES("MOVE_LEFT", "MOVE_RIGHT", "MOVE_UP", "MOVE_DOWN", "EAT",);

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
