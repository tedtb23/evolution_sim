#include "NeuralNet.hpp"
#include "Neuron.hpp"
#include "Genome.hpp"
#include "SDL3/SDL_log.h"
#include <cstdint>
#include <vector>
#include <utility>

float NeuralNet::sigmoid(const float input) {
    return 1.00f / (1.00f + std::exp(-input));
}

float NeuralNet::convertRawWeightOrBias(const uint16_t rawValue) {
    //convert the 16 bit unsigned value to a float between -4.0 and 4.0
    constexpr float stepSize = 8.0f / (UINT16_MAX + 1);
    return stepSize * static_cast<float>(rawValue) - 4.0f;
}

NeuralNet::NeuralNet(const Genome::Genome& genome) {
    for(const auto & [connectionID, rawWeight] : genome.connections) {
        const auto sourceFullID =
            static_cast<uint8_t>(connectionID >> 8);
        const auto destFullID =
            static_cast<uint8_t>(connectionID);
        const bool sourceIsHidden = connectionID & 0x8000;
        const bool destIsHidden = connectionID & 0x0080;
        const uint8_t sourceID = sourceFullID & 0x7F;
        const uint8_t destID = destFullID & 0x7F;
        const float weight = convertRawWeightOrBias(rawWeight);

        std::shared_ptr<Neuron> sourceNeuron = getNeuron(
            sourceIsHidden ?
                NeuronType (static_cast<NeuronHiddenType>(sourceID))
                    :
                NeuronType (static_cast<NeuronInputType>(sourceID)),
            genome.biases.at(sourceFullID));

        std::shared_ptr<Neuron> destNeuron = getNeuron(
            destIsHidden ?
                NeuronType (static_cast<NeuronHiddenType>(destID))
                    :
                NeuronType (static_cast<NeuronOutputType>(destID)),
            genome.biases.at(destFullID));

        destNeuron->prevLayerConnections.value().emplace_back(sourceNeuron, weight);
    }
}

std::vector<std::pair<NeuronInputType, float>> NeuralNet::getInputActivations() const{
    std::vector<std::pair<NeuronInputType, float>> inputActivations;
    inputActivations.reserve(inputNeurons.size());
    for(auto & [neuronID, neuronPtr] : inputNeurons) {
        inputActivations.emplace_back(neuronID, neuronPtr->activation);
    }
    return inputActivations;
}

void NeuralNet::setInputActivations(const std::vector<std::pair<NeuronInputType, float>>& activations) {
    if(activations.size() != inputNeurons.size()) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Activations not provided for every input neuron");
        return;
    }

    for(auto & [neuronID, activation] : activations) {
        if(!inputNeurons.contains(neuronID)) {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR,
                         "Error on provided activation for Neuron ID: %d"
                         "\nError: Provided neuron type is not in the neural network", neuronID);
        }else if(activation < 0.0f || activation > 1.0f) {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR,
                         "Error on provided activation for Neuron ID: %d"
                         "\nError: Provided activation is not between 0.0 and 1.0", neuronID);
        }else {
            inputNeurons[neuronID]->activation = activation;
        }
    }
}

std::vector<std::pair<NeuronOutputType, float>> NeuralNet::getOutputActivations() const{
    std::vector<std::pair<NeuronOutputType, float>> outputActivations;

    feedForward();

    for(auto & [neuronID, neuronPtr] : outputNeurons) {
        outputActivations.emplace_back(neuronID, neuronPtr->activation);
    }

    return outputActivations;
}

std::shared_ptr<Neuron> NeuralNet::getNeuron(
    const NeuronType type,
    const uint16_t rawBias) {

    std::shared_ptr<Neuron> neuron{};

    switch(type.index()) {
        case 0: {
            const auto& neuronID = std::get<NeuronInputType>(type);
            if(!inputNeurons.contains(neuronID)) {
                neuron = createNeuron(type, rawBias);
            }else {
                neuron = inputNeurons[neuronID];
            }
            break;
        }
        case 1: {
            const auto& neuronID = std::get<NeuronHiddenType>(type);
            if(!hiddenNeurons.contains(neuronID)) {
                neuron = createNeuron(type, rawBias);
            }else {
                neuron = hiddenNeurons[neuronID];
            }
            break;
        }
        case 2: {
            const auto& neuronID = std::get<NeuronOutputType>(type);
            if(!outputNeurons.contains(neuronID)) {
                neuron = createNeuron(type, rawBias);
            }else {
                neuron = outputNeurons[neuronID];
            }
            break;
        }
        default: break;
    }
    return neuron;
}

std::shared_ptr<Neuron> NeuralNet::createNeuron(
    const NeuronType type,
    const uint16_t rawBias) {

    auto neuron = std::make_shared<Neuron>(convertRawWeightOrBias(rawBias));

    switch(type.index()) {
        case 0: {
            inputNeurons[std::get<NeuronInputType>(type)] = neuron;
            break;
        }
        case 1: {
            hiddenNeurons[std::get<NeuronHiddenType>(type)] = neuron;
            neuron->prevLayerConnections = std::vector<NeuronConnection>();
            neuron->prevLayerConnections.value().reserve(10);
            break;
        }
        case 2: {
            outputNeurons[std::get<NeuronOutputType>(type)] = neuron;
            neuron->prevLayerConnections = std::vector<NeuronConnection>();
            neuron->prevLayerConnections.value().reserve(10);
            break;
        }
        default: break;
    }
    return neuron;
}

void NeuralNet::feedForward() const {
    for(auto& [neuronID, neuronPtr] : hiddenNeurons) {
        if(neuronPtr->prevLayerConnections.has_value()) {
            for(NeuronConnection& prevConnection : *neuronPtr->prevLayerConnections) {
                const float originalActivation = neuronPtr->activation;
                if(neuronPtr != prevConnection.neuronPtr) {
                    neuronPtr->activation += prevConnection.neuronPtr->activation * prevConnection.weight;
                }else { //if the neuron is connected to itself don't use the intermediate activation
                    neuronPtr->activation += originalActivation * prevConnection.weight;
                }
            }
            neuronPtr->activation += neuronPtr->bias;
            neuronPtr->activation = sigmoid(neuronPtr->activation);
        }
    }
    for(auto& [neuronID, neuronPtr] : outputNeurons) {
        if(neuronPtr->prevLayerConnections.has_value()) {
            for(NeuronConnection& prevConnection : *neuronPtr->prevLayerConnections) {
                neuronPtr->activation += prevConnection.neuronPtr->activation * prevConnection.weight;
            }
            neuronPtr->activation += neuronPtr->bias;
            neuronPtr->activation = sigmoid(neuronPtr->activation);
        }
    }
}