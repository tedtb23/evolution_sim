#ifndef NEURALNET_HPP
#define NEURALNET_HPP
#include "Genome.hpp"
#include "Neuron.hpp"
#include <unordered_map>
#include <vector>
#include <memory>
#include <variant>
#include <utility>

class NeuralNet {
public:
    explicit NeuralNet(const Genome::Genome& genome);
    static float sigmoid(float input);
    [[nodiscard]] std::vector<std::pair<NeuronInputType, float>> getInputActivations() const;
    void setInputActivations(const std::vector<std::pair<NeuronInputType, float>>& activations);
    [[nodiscard]] std::vector<std::pair<NeuronOutputType, float>> getOutputActivations() const;
private:
    using NeuronType = std::variant<NeuronInputType, NeuronHiddenType, NeuronOutputType>;

    static float convertRawWeightOrBias(uint16_t value);
    void feedForward() const;
    std::shared_ptr<Neuron> getNeuron(NeuronType type, uint16_t rawBias);
    std::shared_ptr<Neuron> createNeuron(NeuronType type, uint16_t rawBias);
    std::unordered_map<NeuronInputType, std::shared_ptr<Neuron>> inputNeurons;
    std::unordered_map<NeuronHiddenType, std::shared_ptr<Neuron>> hiddenNeurons;
    std::unordered_map<NeuronOutputType, std::shared_ptr<Neuron>> outputNeurons;
};
#endif //NEURALNET_HPP
