#include "NeuralNet.hpp"
#include "Neuron.hpp"
#include "Genome.hpp"
#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>

NeuralNet::NeuralNet(const Genome::Genome& genome) {
    //inputNeurons.reserve(genome.biases.size() / 2);
    //hiddenNeurons.reserve(genome.biases.size() / 2);
    //outputNeurons.reserve(genome.biases.size() / 2);

    for(const uint32_t& geneConnection : genome.connections) {
        const auto sourceFullID =
            static_cast<uint8_t>((geneConnection & 0xFF000000) >> 24);
        const auto destFullID =
            static_cast<uint8_t>((geneConnection & 0x00FF0000) >> 16);
        const bool sourceIsHidden = geneConnection & 0x80000000;
        const bool destIsHidden = geneConnection & 0x00800000;
        const uint8_t sourceID = sourceFullID & 0x7F;
        const uint8_t destID = destFullID & 0x7F;
        const auto weight = static_cast<uint16_t>(geneConnection);

        std::shared_ptr<Neuron> sourceNeuron = getNeuron(
            sourceIsHidden ?
                std::variant<NeuronInputType, NeuronOutputType, NeuronHiddenType>
                    (static_cast<NeuronHiddenType>(sourceID))
                        :
                std::variant<NeuronInputType, NeuronOutputType, NeuronHiddenType>
                    (static_cast<NeuronInputType>(sourceID)),
            sourceID,
            genome.biases.at(sourceFullID));
        std::shared_ptr<Neuron> destNeuron = getNeuron(
            destIsHidden ?
                std::variant<NeuronInputType, NeuronOutputType, NeuronHiddenType>
                    (static_cast<NeuronHiddenType>(destID))
                        :
                std::variant<NeuronInputType, NeuronOutputType, NeuronHiddenType>
                    (static_cast<NeuronOutputType>(destID)),
            destID,
            genome.biases.at(destFullID));

        sourceNeuron->nextLayerConnections.value().emplace_back(destNeuron, weight);
        destNeuron->prevLayerConnections.value().emplace_back(sourceNeuron, weight);
    }
    for(auto & [val1, val2] : inputNeurons) {
        std::cout << static_cast<int>(val1) << " : " << val2->activation << std::endl;
        std::cout << static_cast<int>(val1) << " : " << val2->bias << std::endl;
    }
    for(auto & [val1, val2] : hiddenNeurons) {
        std::cout << static_cast<int>(val1) << " : " << val2->activation << std::endl;
        std::cout << static_cast<int>(val1) << " : " << val2->bias << std::endl;
    }
    for(auto & [val1, val2] : outputNeurons) {
        std::cout << static_cast<int>(val1) << " : " << val2->activation << std::endl;
        std::cout << static_cast<int>(val1) << " : " << val2->bias << std::endl;
    }
}

std::shared_ptr<Neuron> NeuralNet::getNeuron(
    const std::variant<NeuronInputType, NeuronOutputType, NeuronHiddenType> type,
    const uint8_t id,
    const uint16_t rawBias) {

    std::shared_ptr<Neuron> neuron;

    switch(type.index()) {
        case 0: {
            if(!inputNeurons.contains(id)) {
                neuron = createNeuron(type, id, rawBias);
            }else {
                neuron = inputNeurons[id];
            }
            break;
        }
        case 1: {
            if(!outputNeurons.contains(id)) {
                neuron = createNeuron(type, id, rawBias);
            }else {
                neuron = outputNeurons[id];
            }
            break;
        }
        case 2: {
            if(!hiddenNeurons.contains(id)) {
                neuron = createNeuron(type, id, rawBias);
            }else {
                neuron = hiddenNeurons[id];
            }
            break;
        }
        default: throw std::invalid_argument("Invalid neuron type");
    }
    return neuron;
}

std::shared_ptr<Neuron> NeuralNet::createNeuron(
    const std::variant<NeuronInputType, NeuronOutputType, NeuronHiddenType> type,
    const uint8_t id,
    const uint16_t rawBias) {

    auto neuron = std::make_shared<Neuron>(((8.0f / (UINT16_MAX + 1)) * static_cast<float>(rawBias)) - 4.0f);

    switch(type.index()) {
        case 0: {
            inputNeurons[id] = neuron;
            neuron->nextLayerConnections = std::vector<NeuronConnection>();
            neuron->nextLayerConnections.value().reserve(10);
            break;
        }
        case 1: {
            outputNeurons[id] = neuron;
            neuron->prevLayerConnections = std::vector<NeuronConnection>();
            neuron->prevLayerConnections.value().reserve(10);
            break;
        }
        case 2: {
            hiddenNeurons[id] = neuron;
            neuron->prevLayerConnections = std::vector<NeuronConnection>();
            neuron->nextLayerConnections = std::vector<NeuronConnection>();
            neuron->prevLayerConnections.value().reserve(10);
            neuron->nextLayerConnections.value().reserve(10);
            break;
        }
        default: throw std::invalid_argument("Invalid neuron type");
    }
    return neuron;
}

float NeuralNet::sigmoid(const float input) {
    return 1.00f / (1.00f + std::exp(-input));
}


void NeuralNet::feedForward() const {
    std::vector<std::shared_ptr<Neuron>> stack;
    std::vector<std::shared_ptr<Neuron>> completed;

    for(auto& [id, neuronPtr]: inputNeurons) {
        if(neuronPtr->activation == INFINITY) neuronPtr->activation = 0.0f;
        //assert(neuronPtr->activation != INFINITY);

        for(NeuronConnection & connection : *neuronPtr->nextLayerConnections) {
            if(std::find(stack.begin(), stack.end(), connection.neuronPtr) == stack.end())
                stack.push_back(connection.neuronPtr);
        }
    }

    while(true) {
        while(!stack.empty()) {
            std::shared_ptr<Neuron>& neuronPtr = stack.back();

            for(NeuronConnection & prevConnection : *neuronPtr->prevLayerConnections) {
                neuronPtr->activation += prevConnection.neuronPtr->activation * prevConnection.weight;
            }

            neuronPtr->activation += neuronPtr->bias;
            neuronPtr->activation = sigmoid(neuronPtr->activation);

            completed.push_back(neuronPtr);
            stack.pop_back();
        }
        for(const std::shared_ptr<Neuron>& neuronPtr : completed) {
            if(neuronPtr->nextLayerConnections.has_value())
                for(NeuronConnection & nextConnection : *neuronPtr->nextLayerConnections) {
                    //todo handle case where hidden neurons are connected to themselves or other hidden neurons
                    if(std::find(completed.begin(), completed.end(), nextConnection.neuronPtr) == completed.end())
                        stack.push_back(nextConnection.neuronPtr);
                }
        }
        if(stack.empty()) break;
    }
}
