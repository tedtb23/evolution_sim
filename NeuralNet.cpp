#include "NeuralNet.hpp"

#include <cassert>
#include <vector>

void NeuralNet::calcActivations() {
    std::vector<Neuron*> stack;
    std::vector<Neuron*> completed;

    for(auto & neuron : inputNeurons) {
        assert(neuron.activation != INFINITY);

        for(auto & connection : neuron.nextLayerConnections) {
            if(std::find(stack.begin, stack.end, connection.neuron) == stack.end())
                stack.push_back(connection.neuron);
        }
    }

    while(true) {
        while(!stack.empty()) {
            Neuron &neuron = *stack.back();

            for(auto & prevConnection : neuron.prevLayerConnections) {
                Neuron &prevNeuron = *prevConnection.neuron;
                neuron.activation += prevNeuron.activation * prevConnection.weight;
            }

            neuron.activation += neuron.bias;
            neuron.activation = sigmoid(neuron.activation);

            completed.push_back(&neuron);
            stack.pop_back();
        }
        for(auto & neuron : completed) {
            for(auto & nextConnection : neuron->nextLayerConnections) {
                if(std::find(stack.begin(), stack.end, nextConnection.neuron) == stack.end())
                    stack.push_back(nextConnection.neuron);
            }
        }
        if(stack.empty()) break;
    }
}