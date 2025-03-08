#ifndef NEURALNET_HPP
#define NEURALNET_HPP
#include "Genome.hpp"
#include "Neuron.hpp"
#include <unordered_map>
#include <vector>
#include <utility>
#include <memory>
#include <variant>


class NeuralNet {
public:
    explicit NeuralNet(const Genome::Genome& genome);
    [[nodiscard]] std::vector<std::pair<NeuronInputType, float>> getInputActivations() const;
    void setInputActivations(const std::vector<std::pair<NeuronInputType, float>>& activations);
    [[nodiscard]] std::vector<std::pair<NeuronOutputType, float>> getOutputActivations() const;
private:
    static float sigmoid(float input);
    static float convertRawWeightOrBias(uint16_t value);
    void feedForward() const;
    std::shared_ptr<Neuron> getNeuron(
        std::variant<NeuronInputType, NeuronHiddenType, NeuronOutputType> type,
        uint16_t rawBias);
    std::shared_ptr<Neuron> createNeuron(
        std::variant<NeuronInputType, NeuronHiddenType, NeuronOutputType> type,
        uint16_t rawBias);
    std::unordered_map<NeuronInputType, std::shared_ptr<Neuron>> inputNeurons;
    std::unordered_map<NeuronHiddenType, std::shared_ptr<Neuron>> hiddenNeurons;
    std::unordered_map<NeuronOutputType, std::shared_ptr<Neuron>> outputNeurons;
};
#endif //NEURALNET_HPP
