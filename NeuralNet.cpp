#include "NeuralNet.hpp"
#include "Neuron.hpp"
#include "Genome.hpp"
#include "SDL3/SDL_log.h"
#include <cassert>
#include <cstdint>
#include <vector>

float NeuralNet::sigmoid(const float input) {
    return 1.00f / (1.00f + std::exp(-input));
}

float NeuralNet::convertRawWeightOrBias(const uint16_t rawValue) {
    constexpr float stepSize = 8.0f / (UINT16_MAX + 1);
    return stepSize * static_cast<float>(rawValue) - 4.0f;
}

NeuralNet::NeuralNet(const Genome::Genome& genome) {
    //inputNeurons.reserve(genome.biases.size() / 2);
    //hiddenNeurons.reserve(genome.biases.size() / 2);
    //outputNeurons.reserve(genome.biases.size() / 2);

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
                std::variant<NeuronInputType, NeuronHiddenType, NeuronOutputType>
                    (static_cast<NeuronHiddenType>(sourceID))
                        :
                std::variant<NeuronInputType, NeuronHiddenType, NeuronOutputType>
                    (static_cast<NeuronInputType>(sourceID)),
            genome.biases.at(sourceFullID));

        std::shared_ptr<Neuron> destNeuron = getNeuron(
            destIsHidden ?
                std::variant<NeuronInputType, NeuronHiddenType, NeuronOutputType>
                    (static_cast<NeuronHiddenType>(destID))
                        :
                std::variant<NeuronInputType, NeuronHiddenType, NeuronOutputType>
                    (static_cast<NeuronOutputType>(destID)),
            genome.biases.at(destFullID));

        //sourceNeuron->nextLayerConnections.value().emplace_back(destNeuron, weight);
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

//removes any hidden neurons that don't eventually connect to an input and output.
void NeuralNet::validate() {
    /*
    std::set<std::shared_ptr<Neuron>> set;
    for(auto & [neuronID, neuronPtr] : inputNeurons) {
        if(neuronPtr->nextLayerConnections.has_value()) //maybe not needed as input neurons should always have next layer connections or they wouldn't exist.
            for(NeuronConnection& connection : *neuronPtr->nextLayerConnections) {
                if(connection.neuronPtr->type.index() != 1) {
                    set.insert(neuronPtr);
                }
            }
    }
    while(!set.empty()) {
        const std::shared_ptr<Neuron> &neuronPtr = *set.begin();
        if (neuronPtr->nextLayerConnections.has_value())
            for (NeuronConnection &connection: *neuronPtr->nextLayerConnections)
                if (connection.neuronPtr->type.index() != 1 && neuronPtr != connection.neuronPtr)
                    set.insert(neuronPtr);
        set.erase(set.begin());
    }

    for(auto itr = hiddenNeurons.begin(); itr != hiddenNeurons.end();) {
        const std::shared_ptr<Neuron>& neuronPtr = itr->second;
        if(!set.contains(neuronPtr)) {
            if(neuronPtr->prevLayerConnections.has_value())
                for(auto jtr = neuronPtr->prevLayerConnections->begin(); jtr != neuronPtr->prevLayerConnections->end();) {
                    auto& jeuronPtr = *jtr->neuronPtr;
                    if(jeuronPtr.nextLayerConnections.has_value())
                        jeuronPtr.nextLayerConnections->erase(std::remove(jeuronPtr.nextLayerConnections.value().begin(), jeuronPtr.nextLayerConnections.value().end(), jeuronPtr), jeuronPtr.nextLayerConnections->end());
                }

            itr = hiddenNeurons.erase(itr);
        }else {
            ++itr;
        }
    }

    for(auto & [neuronID, neuronPtr] : outputNeurons) {
        if(neuronPtr->prevLayerConnections.has_value()) //maybe not needed as input neurons should always have next layer connections or they wouldn't exist.
            for(NeuronConnection& connection : *neuronPtr->prevLayerConnections) {
                if(connection.neuronPtr->type.index() != 0) {
                    set.insert(neuronPtr);
                }
            }
    }
    while(!set.empty()) {
        const std::shared_ptr<Neuron>& neuronPtr = *set.begin();
        if(neuronPtr->prevLayerConnections.has_value())
            for(NeuronConnection& connection : *neuronPtr->prevLayerConnections)
                if(connection.neuronPtr->type.index() != 0 && neuronPtr != connection.neuronPtr)
                    set.insert(neuronPtr);
        set.erase(set.begin());
    }
     */
}

std::shared_ptr<Neuron> NeuralNet::getNeuron(
    const std::variant<NeuronInputType, NeuronHiddenType, NeuronOutputType> type,
    const uint16_t rawBias) {

    std::shared_ptr<Neuron> neuron{};

    switch(type.index()) {
        case 0: {
            const NeuronInputType neuronID = std::get<NeuronInputType>(type);
            if(!inputNeurons.contains(neuronID)) {
                neuron = createNeuron(type, rawBias);
            }else {
                neuron = inputNeurons[neuronID];
            }
            break;
        }
        case 1: {
            const NeuronHiddenType neuronID = std::get<NeuronHiddenType>(type);
            if(!hiddenNeurons.contains(neuronID)) {
                neuron = createNeuron(type, rawBias);
            }else {
                neuron = hiddenNeurons[neuronID];
            }
            break;
        }
        case 2: {
            const NeuronOutputType neuronID = std::get<NeuronOutputType>(type);
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
    const std::variant<NeuronInputType, NeuronHiddenType, NeuronOutputType> type,
    const uint16_t rawBias) {

    auto neuron = std::make_shared<Neuron>(convertRawWeightOrBias(rawBias));

    switch(type.index()) {
        case 0: {
            inputNeurons[std::get<NeuronInputType>(type)] = neuron;
            //neuron->nextLayerConnections = std::vector<NeuronConnection>();
            //neuron->nextLayerConnections.value().reserve(10);
            break;
        }
        case 1: {
            hiddenNeurons[std::get<NeuronHiddenType>(type)] = neuron;
            neuron->prevLayerConnections = std::vector<NeuronConnection>();
            //neuron->nextLayerConnections = std::vector<NeuronConnection>();
            neuron->prevLayerConnections.value().reserve(10);
            //neuron->nextLayerConnections.value().reserve(10);
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
    for(auto & [neuronID, neuronPtr] : hiddenNeurons) {
        if(neuronPtr->prevLayerConnections.has_value()) { //maybe not needed as the optional for prevLayerConnections for hidden layer neurons will always be initialized during creation of the neuron
            for(NeuronConnection& prevConnection : *neuronPtr->prevLayerConnections) {
                const float originalActivation = neuronPtr->activation;
                if(neuronPtr != prevConnection.neuronPtr) {
                    neuronPtr->activation += prevConnection.neuronPtr->activation * prevConnection.weight;
                }else {
                    neuronPtr->activation += originalActivation * prevConnection.weight;
                }
            }
            neuronPtr->activation += neuronPtr->bias;
            neuronPtr->activation = sigmoid(neuronPtr->activation);
        }
    }
    for(auto & [neuronID, neuronPtr] : outputNeurons) {
        //if (neuronPtr->prevLayerConnections.has_value()) {
            for (NeuronConnection &prevConnection: *neuronPtr->prevLayerConnections) {
                neuronPtr->activation += prevConnection.neuronPtr->activation * prevConnection.weight;
            }
            neuronPtr->activation += neuronPtr->bias;
            neuronPtr->activation = sigmoid(neuronPtr->activation);
        //}
    }
}