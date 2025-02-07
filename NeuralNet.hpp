#ifndef NEURALNET_H
#define NEURALNET_H
#include <vector>

struct Neuron;
struct NeuronConnection;

enum NeuronCategory {
    NEURON_INPUT,
    NEURON_OUTPUT,
    NEURON_HIDDEN
};

enum NeuronType {
    //input types
    SIGHT_OBJECT_FORWARD,

    //output types
    MOVE_LEFT,
};

struct Neuron {
    NeuronCategory category;
    NeuronType type;
    float activation = INFINITY;
    float bias = NULL;
    std::vector<NeuronConnection> prevLayerConnections;
    std::vector<NeuronConnection> nextLayerConnections;
};

struct NeuronConnection {
    Neuron* neuron;
    float weight;
};

class NeuralNet {
public:
    explicit NeuralNet(std::vector<uint32_t> genome);
    ~NeuralNet();
    std::vector<NeuronType> getInputNeuronTypes();
    void setInputActivations(std::vector<float> activations);
    std::vector<NeuronType> getOutputActions();
private:
    float sigmoid(float input);
    void calcActivations();
    std::vector<Neuron> inputNeurons;
    std::vector<Neuron> outputNeurons;
};




#endif //NEURALNET_H
