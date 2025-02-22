#ifndef GENOME_HPP
#define GENOME_HPP
#include "Neuron.hpp"
#include <cassert>
#include <cstdint>
#include <vector>
#include <random>
#include <unordered_set>

namespace Genome {

    static uint8_t inputNeuronTypesSize = static_cast<uint8_t>(NeuronInputType::SIZE);
    static uint8_t hiddenNeuronTypesSize = static_cast<uint8_t>(NeuronHiddenType::SIZE);
    static uint8_t outputNeuronTypesSize = static_cast<uint8_t>(NeuronOutputType::SIZE);


    struct Genome {
        // [source neuron is hidden | source neuron id , destination neuron is hidden | destination neuron id ] -> weight
        // [0                       | 0000000          , 0                            | 0000000               ] -> 16bits
        std::unordered_map<std::pair<uint8_t, uint8_t>, uint16_t> connections;

        //neuron id -> bias
        //00000000  -> 16bits
        std::unordered_map<uint8_t, uint16_t> biases;
    };

    //returns a random value within the range specified by start and end (both being inclusive).
    //end must be greater than start.
    //the type of the parameters must be an integer type or can be easily cast to an integer.
    //returned values are part of a uniform integer distribution within the range.
    template <typename T>
    static T getRandomValue(T start, T end) {
        assert(end > start);
        std::random_device rd;
        std::mt19937 mt(rd());
        std::uniform_int_distribution<T> distribution(start, end);
        return distribution(mt);
    }

    static uint8_t getRandomNeuronID(const bool isSource) {
        std::random_device rd;
        std::mt19937 mt(rd());
        std::uniform_int_distribution<uint8_t> distHidden(false, true);
        std::uniform_int_distribution<uint8_t> distNeuronID;

        const bool isHidden = static_cast<bool>(distHidden(mt));

        if(isHidden)
            distNeuronID.param(std::uniform_int_distribution<uint8_t>::param_type(0,
                hiddenNeuronTypesSize - 1));
        else
            if(isSource)
                distNeuronID.param(std::uniform_int_distribution<uint8_t>::param_type(
                    0,
                    inputNeuronTypesSize - 1));
            else
                distNeuronID.param(std::uniform_int_distribution<uint8_t>::param_type(
                    inputNeuronTypesSize, //maybe change range to 0-NeuronOutputType::SIZE - 1
                    inputNeuronTypesSize + outputNeuronTypesSize - 1));

        uint8_t neuronID = static_cast<uint8_t>(isHidden) << 7;
        neuronID |= distNeuronID(mt);

        return neuronID;
    }

    static uint16_t getRandomWeightOrBias() {
        std::random_device rd;
        std::mt19937 mt(rd());
        std::uniform_int_distribution<uint16_t> distWeightBias(0, UINT16_MAX);

        return distWeightBias(mt);
    }

    inline Genome createRandomGenome(const int size) {
        assert(size > 0); //&& size < inputNeuronTypesSize + hiddenNeuronTypesSize + outputNeuronTypesSize);

        Genome genome {
            .connections = std::unordered_map<std::pair<uint8_t, uint8_t>, uint16_t>(),
            .biases = std::unordered_map<uint8_t, uint16_t>()
        };
        genome.connections.reserve(size);

        for (int i = 0; i < size; i++) {
            const uint8_t sourceID = getRandomNeuronID(true);
            const uint8_t destID = getRandomNeuronID(false);
            const uint8_t weight = getRandomWeightOrBias();

            if(genome.connections.contains(std::make_pair(sourceID, destID))) continue;

            genome.connections[std::make_pair(sourceID, destID)] = weight;

            if(!genome.biases.contains(sourceID))
                genome.biases[sourceID] = getRandomWeightOrBias();

            if(!genome.biases.contains(destID))
                genome.biases[destID] = getRandomWeightOrBias();
        }
        return genome;
    }

    inline Genome createGenomeFromParents(const Genome& parent1, const Genome& parent2) {
        const Genome *largerParentPtr = nullptr;
        const Genome *smallerParentPtr = nullptr;
        if(parent1.connections.size() >= parent2.connections.size()) {
            largerParentPtr = &parent1;
            smallerParentPtr = &parent2;
        }else {
            largerParentPtr = &parent2;
            smallerParentPtr = &parent1;
        }
        const Genome& largerParent = *largerParentPtr;
        const Genome& smallerParent = *smallerParentPtr;

        Genome genome {
            .connections = std::unordered_map<std::pair<uint8_t, uint8_t>, uint16_t>(),
            .biases = std::unordered_map<uint8_t, uint16_t>()};
        genome.connections.reserve(largerParent.connections.size());

        std::random_device rd;
        std::mt19937 mt(rd());
        std::uniform_int_distribution<int> distNumSmallerParentGenes(1, static_cast<int>(smallerParent.connections.size()) - 2);
        std::uniform_int_distribution<uint8_t> distWhichParentFirst(true, false);

        const int r = distNumSmallerParentGenes(mt);
        const bool smallerParentFirst = static_cast<bool>(distWhichParentFirst(mt));

        int i = 0;
        const auto itrL = largerParent.connections.begin();
        const auto itrS = smallerParent.connections.begin();
        auto currItr = itrL;
        const Genome* currParentPtr = &largerParent;

        while(itrL != largerParent.connections.end()) {
            if(
                (smallerParentFirst && i < r)
                    ||
                (!smallerParentFirst && i >= largerParent.connections.size() - r)) {

                currItr = itrS;
                currParentPtr = &smallerParent;
            }else if(
                (smallerParentFirst && i >= r)
                    ||
                (!smallerParentFirst && i < largerParent.connections.size() - r)) {

                currItr = itrL;
                currParentPtr = &largerParent;
            }

            genome.connections[currItr->first] = currItr->second;
            ++currItr;

            if(!genome.biases.contains(currItr->first.first))
                genome.biases[currItr->first.first] = currParentPtr->biases.at(currItr->first.first);

            if(!genome.biases.contains(currItr->first.second))
                genome.biases[currItr->first.second] = currParentPtr->biases.at(currItr->first.second);
            i++;
            }
        return genome;
    }


    inline void mutateGenome(Genome* genomePtr) {
        Genome& genome = *genomePtr;

        std::random_device rd;
        std::mt19937 mt(rd());
        std::geometric_distribution<int> distNumConnectionsToMutate(0.2);
        std::uniform_int_distribution<int> distConnectionsToMutate(0, static_cast<int>(genome.connections.size()) - 1);
        int numConnectionsToMutate;
        std::unordered_set<int> connectionsToMutate;

        do {
            numConnectionsToMutate = distNumConnectionsToMutate(mt);
        } while(numConnectionsToMutate < 1 && numConnectionsToMutate > genome.connections.size() - 1);

        for(int i = 0; i < numConnectionsToMutate; i++) {
            connectionsToMutate.insert(distConnectionsToMutate(mt));
        }

        for(const int connectionIDX : connectionsToMutate) {
            uint32_t *connection = &genome.connections[connectionIDX];

            //mutation type of 0 denotes a source neuron mutation,
            //1 denotes a destination neuron mutation, 2 denotes a weight mutation,
            //3 denotes a bias mutation, and 4 denotes a whole gene mutation
            switch(std::uniform_int_distribution<uint8_t> distMutationType(0, 4); distMutationType(mt)) {
                //source neuron mutation
                case 0: {
                    const uint8_t sourceID = setRandomNeuronID(connection, true);

                    if(!genome.biases.contains(sourceID))
                        genome.biases[sourceID] = getRandomBias();

                    break;
                }
                //destination neuron mutation
                case 1: {
                    const uint8_t destID = setRandomNeuronID(connection, false);

                    if(!genome.biases.contains(destID))
                        genome.biases[destID] = getRandomBias();

                    break;
                }
                //weight mutation
                case 2: {
                    setRandomWeight(connection);
                    break;
                }
                //bias mutation
                case 3: {
                    const auto sourceID =
                        static_cast<uint8_t>((*connection & 0xFF000000) >> 24);
                    const auto destID =
                        static_cast<uint8_t>((*connection & 0x00FF0000) >> 16);

                    genome.biases[sourceID] = getRandomBias();
                    genome.biases[destID] = getRandomBias();
                    break;
                }
                //mutate whole gene
                case 4: {
                    uint8_t sourceID = setRandomNeuronID(connection, true);
                    uint8_t destID = setRandomNeuronID(connection, false);
                    setRandomWeight(connection);

                    genome.biases[sourceID] = getRandomBias();
                    genome.biases[destID] = getRandomBias();

                    break;
                }
                default: break;
            }
        }
    }
}
#endif //GENOME_HPP