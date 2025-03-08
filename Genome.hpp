#ifndef GENOME_HPP
#define GENOME_HPP
#include "Neuron.hpp"
#include <cassert>
#include <cstdint>
#include <random>
#include <set>
#include <unordered_map>

namespace Genome {
    struct Genome {
        // [source neuron is hidden | source neuron id | destination neuron is hidden | destination neuron id ] -> weight
        // [0                       | 0000000          | 0                            | 0000000               ] -> 16bits
        std::unordered_map<uint16_t, uint16_t> connections;

        //neuron id -> bias
        //00000000  -> 16bits
        std::unordered_map<uint8_t, uint16_t> biases;
    };

    static inline uint8_t getRandomNeuronID(const bool isSource) {
        std::random_device rd;
        std::mt19937 mt(rd());
        std::uniform_int_distribution<uint8_t> distHidden(false, true);
        std::uniform_int_distribution<uint8_t> distNeuronID;

        const bool isHidden = static_cast<bool>(distHidden(mt));

        if(isHidden)
            distNeuronID.param(std::uniform_int_distribution<uint8_t>::param_type(0,
                NEURONHIDDENTYPE_SIZE - 1));
        else
            if(isSource)
                distNeuronID.param(std::uniform_int_distribution<uint8_t>::param_type(
                    0,
                    NEURONINPUTTYPE_SIZE - 1));
            else
                distNeuronID.param(std::uniform_int_distribution<uint8_t>::param_type(
                    NEURONINPUTTYPE_SIZE, //maybe change range to 0-NeuronOutputType::SIZE - 1
                    NEURONINPUTTYPE_SIZE + NEURONOUTPUTTYPE_SIZE - 1));

        uint8_t neuronID = static_cast<uint8_t>(isHidden) << 7;
        neuronID |= distNeuronID(mt);

        return neuronID;
    }

    static inline uint16_t getRandomConnectionID() {
        auto connection = static_cast<uint16_t>(getRandomNeuronID(true)) << 8;
        connection |= static_cast<uint16_t>(getRandomNeuronID(false));
        return connection;
    }

    static inline uint16_t getRandomWeightOrBias() {
        std::random_device rd;
        std::mt19937 mt(rd());
        std::uniform_int_distribution<uint16_t> distWeightBias(0, UINT16_MAX);

        return distWeightBias(mt);
    }

    inline Genome createRandomGenome(const int size) {
        assert(size > 0 && size <= 1000);

        Genome genome {
            .connections = std::unordered_map<uint16_t, uint16_t>(),
            .biases = std::unordered_map<uint8_t, uint16_t>()
        };
        genome.connections.reserve(size);

        for (int i = 0; i < size; i++) {
            const uint16_t connectionID = getRandomConnectionID();
            if(genome.connections.contains(connectionID)) continue;
            const auto sourceID = static_cast<uint8_t>(connectionID >> 8);
            const auto destID = static_cast<uint8_t>(connectionID);
            const uint8_t weight = getRandomWeightOrBias();

            genome.connections[connectionID] = weight;

            if(!genome.biases.contains(sourceID))
                genome.biases[sourceID] = getRandomWeightOrBias();

            if(!genome.biases.contains(destID))
                genome.biases[destID] = getRandomWeightOrBias();
        }
        return genome;
    }

    inline Genome createGenomeFromParents(const Genome& parent1, const Genome& parent2) {
        const bool p1IsLarger = parent1.connections.size() >= parent2.connections.size();
        const Genome *largerParentPtr = p1IsLarger ? &parent1 : &parent2;
        const Genome *smallerParentPtr = p1IsLarger ? &parent2 : &parent1;

        Genome genome {
            .connections = std::unordered_map<uint16_t , uint16_t>(),
            .biases = std::unordered_map<uint8_t, uint16_t>()};
        genome.connections.reserve(largerParentPtr->connections.size());

        std::uniform_int_distribution<int> distNumSmallerParentGenes(1, static_cast<int>(smallerParentPtr->connections.size()) - 2);
        std::uniform_int_distribution<uint8_t> distWhichParentFirst(true, false);

        std::random_device rd;
        std::mt19937 mt(rd());
        const int numSmallerParentGenes = distNumSmallerParentGenes(mt);
        const bool smallerParentFirst = static_cast<bool>(distWhichParentFirst(mt));

        int i = 0;
        auto itrL = largerParentPtr->connections.begin();
        auto itrS = smallerParentPtr->connections.begin();
        auto* currItr = &itrL;
        const Genome* currParentPtr = largerParentPtr;

        while(itrL != largerParentPtr->connections.end()) {
            if(
                (smallerParentFirst && i < numSmallerParentGenes)
                    ||
                (!smallerParentFirst && i >= largerParentPtr->connections.size() - numSmallerParentGenes)) {

                currItr = &itrS;
                currParentPtr = smallerParentPtr;
            }else if(
                (smallerParentFirst && i >= numSmallerParentGenes)
                    ||
                (!smallerParentFirst && i < largerParentPtr->connections.size() - numSmallerParentGenes)) {

                currItr = &itrL;
                currParentPtr = largerParentPtr;
            }

            genome.connections[(*currItr)->first] = (*currItr)->second;

            const auto sourceID = static_cast<uint8_t>((*currItr)->first >> 8);
            const auto destID = static_cast<uint8_t>((*currItr)->first);
            if(!genome.biases.contains(sourceID))
                genome.biases[sourceID] = currParentPtr->biases.at(sourceID);

            if(!genome.biases.contains(destID))
                genome.biases[destID] = currParentPtr->biases.at(destID);

            ++(*currItr);
            i++;
            }
        return genome;
    }

    static inline void mutateGene(Genome* genomePtr, const uint16_t connectionID) {
        const auto sourceID = static_cast<uint8_t>(connectionID >> 8);
        const auto destID = static_cast<uint8_t>(connectionID);
        const uint16_t weight = genomePtr->connections[connectionID];

        std::random_device rd;
        std::mt19937 mt(rd());
        //mutation type of 0 denotes a source neuron mutation,
        //1 denotes a destination neuron mutation, 2 denotes a weight mutation,
        //3 denotes a bias mutation, and 4 denotes a whole gene mutation
        switch(std::uniform_int_distribution<uint8_t> distMutationType(0, 4); distMutationType(mt)) {
            case 0: { //source neuron mutation
                const uint8_t newSourceID = getRandomNeuronID(true);
                const uint16_t newConnectionID = (connectionID & 0x00FF) | (static_cast<uint16_t>(newSourceID) << 8);

                if(!genomePtr->connections.contains(newConnectionID)) {
                    genomePtr->connections.erase(connectionID);
                    genomePtr->connections[newConnectionID] = weight;
                    if(!genomePtr->biases.contains(newSourceID))
                        genomePtr->biases[newSourceID] = getRandomWeightOrBias();
                }
                break;
            }
            case 1: { //destination neuron mutation
                const uint8_t newDestID = getRandomNeuronID(false);
                const uint16_t newConnectionID = (connectionID & 0xFF00) | static_cast<uint16_t>(newDestID);

                if(!genomePtr->connections.contains(newConnectionID)) {
                    genomePtr->connections.erase(connectionID);
                    genomePtr->connections[newConnectionID] = weight;
                    if(!genomePtr->biases.contains(newDestID))
                        genomePtr->biases[newDestID] = getRandomWeightOrBias();
                }
                break;
            }
            case 2: { //weight mutation
                genomePtr->connections[connectionID] = getRandomWeightOrBias();
                break;
            }
                //bias mutation
            case 3: {
                genomePtr->biases[sourceID] = getRandomWeightOrBias();
                genomePtr->biases[destID] = getRandomWeightOrBias();
                break;
            }
            case 4: { //mutate whole gene
                const uint16_t newConnectionID = getRandomConnectionID();

                if(!genomePtr->connections.contains(newConnectionID)) {
                    genomePtr->connections.erase(connectionID);
                    genomePtr->connections[newConnectionID] = getRandomWeightOrBias();
                    const auto newSourceID = static_cast<uint8_t>(newConnectionID >> 8);
                    const auto newDestID = static_cast<uint8_t>(newConnectionID);
                    genomePtr->biases[newSourceID] = getRandomWeightOrBias();
                    genomePtr->biases[newDestID] = getRandomWeightOrBias();
                }else {
                    genomePtr->connections[connectionID] = getRandomWeightOrBias();
                    genomePtr->biases[sourceID] = getRandomWeightOrBias();
                    genomePtr->biases[destID] = getRandomWeightOrBias();
                }
                break;
            }
            default: break;
        }
    }

    inline void mutateGenome(Genome* genomePtr) {
        std::random_device rd;
        std::mt19937 mt(rd());
        std::geometric_distribution<int> distNumConnectionsToMutate(0.2);
        std::uniform_int_distribution<int> distConnectionsToMutate(0, static_cast<int>(genomePtr->connections.size()) - 1);
        int numConnectionsToMutate;
        std::set<int> connectionsToMutate;

        do {
            numConnectionsToMutate = distNumConnectionsToMutate(mt);
        } while(numConnectionsToMutate < 1 || numConnectionsToMutate > genomePtr->connections.size() - 1);

        for(int i = 0; i < numConnectionsToMutate; i++) {
            connectionsToMutate.insert(distConnectionsToMutate(mt));
        }

        int i = 0;
        auto itrConnections = genomePtr->connections.begin();
        auto itrConnectionsToMutate = connectionsToMutate.begin();
        while(itrConnections != genomePtr->connections.end() && itrConnectionsToMutate != connectionsToMutate.end()) {
            if(i != *itrConnectionsToMutate) {
                ++i;
                ++itrConnections;
                continue;
            }
            ++i;
            ++itrConnectionsToMutate;

            const uint16_t connectionID = itrConnections->first;
            mutateGene(genomePtr, connectionID);
        }
    }
}
#endif //GENOME_HPP