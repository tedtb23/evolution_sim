#ifndef NEURON_HPP
#define NEURON_HPP

#include <array>
#include <memory>
#include <optional>
#include <vector>

constexpr std::array<const char*, 10> hiddenValues = {"ZERO","ONE","TWO","THREE","FOUR","FIVE","SIX","SEVEN","EIGHT","NINE"};
constexpr std::array<const char*, 23> inputValues = {
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
};
constexpr std::array<const char*, 5> outputValues = {"MOVE_LEFT", "MOVE_RIGHT", "MOVE_UP", "MOVE_DOWN", "EAT",};

#define NEURONHIDDENTYPE_VALUES hiddenValues;
#define NEURONINPUTTYPE_VALUES inputValues;
#define NEURONOUTPUTTYPE_VALUES outputValues;

inline size_t NEURONHIDDENTYPE_SIZE = hiddenValues.size();
inline size_t NEURONINPUTTYPE_SIZE = inputValues.size();
inline size_t NEURONOUTPUTTYPE_SIZE = outputValues.size();

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

enum NeuronOutputType {
    MOVE_LEFT = HYDROGEN_SATURATION + 1,
    MOVE_RIGHT,
    MOVE_UP,
    MOVE_DOWN,
    EAT,
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
    float activation = 0.0f;
    float bias;
    std::optional<std::vector<NeuronConnection>> prevLayerConnections;

    explicit Neuron(const float bias) : bias(bias) {}
};

#endif //NEURON_HPP

