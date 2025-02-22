#ifndef NEURALNET_HPP
#define NEURALNET_HPP
#include "Genome.hpp"
#include "Neuron.hpp"
#include <unordered_map>
#include <vector>
#include <memory>
#include <variant>


class NeuralNet {
public:
    explicit NeuralNet(const Genome::Genome& genome);
    //~NeuralNet();
    const std::vector<NeuronInputType>& getInputNeuronTypes();
    void setInputActivations(std::vector<float> activations);
    const std::vector<NeuronOutputType>& getOutputActions();
private:
    static float sigmoid(float input);
    void feedForward() const;
    std::shared_ptr<Neuron> getNeuron(
        std::variant<NeuronInputType, NeuronOutputType, NeuronHiddenType> type,
        uint8_t id,
        uint16_t rawBias);
    std::shared_ptr<Neuron> createNeuron(
        std::variant<NeuronInputType, NeuronOutputType, NeuronHiddenType> type,
        uint8_t id,
        uint16_t rawBias);
    std::unordered_map<uint8_t, std::shared_ptr<Neuron>> inputNeurons;
    std::unordered_map<uint8_t, std::shared_ptr<Neuron>> hiddenNeurons;
    std::unordered_map<uint8_t, std::shared_ptr<Neuron>> outputNeurons;
};
#endif //NEURALNET_HPP
