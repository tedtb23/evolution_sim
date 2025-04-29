#ifndef GENOME_HPP
#define GENOME_HPP
#include "Neuron.hpp"
#include "Traits.hpp"
#include <cassert>
#include <cstdint>
#include <random>
#include <set>
#include <unordered_map>
#include <array>

namespace Genome {

    static std::mt19937 mt{std::random_device{}()};

    struct Genome {
        // [source neuron is hidden | source neuron id | destination neuron is hidden | destination neuron id ] -> weight
        // [0                       | 0000000          | 0                            | 0000000               ] -> 16bits
        std::unordered_map<uint16_t, uint16_t> connections;

        //neuron id -> bias
        //00000000  -> 16bits
        std::unordered_map<uint8_t, uint16_t> biases;
    };

    struct TraitGenome {
        //index corresponds to trait id, uint16_t corresponds to trait value.
        std::array<uint16_t, TRAITS_SIZE> traits;
    };

    static inline uint8_t getRandomNeuronID(const bool isSource) {
        std::bernoulli_distribution distHidden(0.50);
        std::uniform_int_distribution<uint8_t> distNeuronID;

        const bool isHidden = distHidden(mt);

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
                    NEURONINPUTTYPE_SIZE,
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

    static inline uint16_t getRandomValue() {
        std::uniform_int_distribution<uint16_t> distValue(0, UINT16_MAX);

        return distValue(mt);
    }

    inline TraitGenome createRandomTraitGenome() {
        TraitGenome traitGenome = {};

        for(auto& trait : traitGenome.traits) {
            trait = getRandomValue();
        }

        return traitGenome;
    }

    inline Genome createRandomGenome(const uint16_t size) {
        assert(size > 0 && size <= 1000);

        Genome genome{};
        genome.connections.reserve(size);

        for (int i = 0; i < size; i++) {
            const uint16_t connectionID = getRandomConnectionID();
            if(genome.connections.contains(connectionID)) continue;
            const auto sourceID = static_cast<uint8_t>(connectionID >> 8);
            const auto destID = static_cast<uint8_t>(connectionID);
            const uint16_t weight = getRandomValue();

            genome.connections[connectionID] = weight;

            if(!genome.biases.contains(sourceID))
                genome.biases[sourceID] = getRandomValue();

            if(!genome.biases.contains(destID))
                genome.biases[destID] = getRandomValue();
        }
        return genome;
    }

    inline TraitGenome createTraitGenomeFromParents(const TraitGenome& parent1, const TraitGenome& parent2) {
        assert(parent1.traits.size() == parent2.traits.size());
        const size_t size = parent1.traits.size();
        TraitGenome traitGenome{};

        std::uniform_int_distribution<int> distNumParent1Genes(1, static_cast<int>(size) - 2);
        std::bernoulli_distribution distWhichParentFirst(0.50f);

        const int numParent1Genes = distNumParent1Genes(mt);
        const bool parent1First = distWhichParentFirst(mt);

        for(int i = 0; i < size; i++) {
            if((parent1First && i < numParent1Genes) || (!parent1First && i >= size - numParent1Genes)) {
                traitGenome.traits[i] = parent1.traits[i];
            }else {
                traitGenome.traits[i] = parent2.traits[i];
            }
        }
        return traitGenome;
    }

    inline Genome createGenomeFromParents(const Genome& parent1, const Genome& parent2) {
        const bool p1IsLarger = parent1.connections.size() >= parent2.connections.size();
        const Genome *largerParentPtr = p1IsLarger ? &parent1 : &parent2;
        const Genome *smallerParentPtr = p1IsLarger ? &parent2 : &parent1;

        Genome genome{};
        genome.connections.reserve(largerParentPtr->connections.size());

        std::uniform_int_distribution<int> distNumSmallerParentGenes(1, static_cast<int>(smallerParentPtr->connections.size()) - 2);
        std::bernoulli_distribution distWhichParentFirst(0.50f);

        const int numSmallerParentGenes = distNumSmallerParentGenes(mt);
        const bool smallerParentFirst = distWhichParentFirst(mt);

        if(parent1.connections.size() <= 2 || parent2.connections.size() <= 2) {
            if(smallerParentFirst) {
                genome.connections.insert(parent1.connections.begin(), parent1.connections.end());
                genome.connections.insert(parent2.connections.begin(), parent2.connections.end());
                genome.biases.insert(parent2.biases.begin(), parent2.biases.end());
                genome.biases.insert(parent1.biases.begin(), parent1.biases.end());
            }else{
                genome.connections.insert(parent2.connections.begin(), parent2.connections.end());
                genome.connections.insert(parent1.connections.begin(), parent1.connections.end());
                genome.biases.insert(parent1.biases.begin(), parent1.biases.end());
                genome.biases.insert(parent2.biases.begin(), parent2.biases.end());
            }
            return genome;
        }

        int i = 0;
        auto itrL = largerParentPtr->connections.begin();
        auto itrS = smallerParentPtr->connections.begin();
        auto* currItr = &itrL;
        const Genome* currParentPtr = largerParentPtr;
        if(smallerParentFirst) {
            std::advance(itrL, numSmallerParentGenes);
        }else {
            std::advance(itrS, smallerParentPtr->connections.size() - numSmallerParentGenes);
        }

        while(itrL != largerParentPtr->connections.end() && itrS != smallerParentPtr->connections.end()) {
            if(
                smallerParentFirst && i < numSmallerParentGenes
                ||
                !smallerParentFirst && i >= largerParentPtr->connections.size() - numSmallerParentGenes
            ) {
                currItr = &itrS;
                currParentPtr = smallerParentPtr;
            }else if(
                smallerParentFirst && i >= numSmallerParentGenes
                ||
                !smallerParentFirst && i < largerParentPtr->connections.size() - numSmallerParentGenes
            ) {
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
            ++i;
        }
        return genome;
    }

    static inline void mutateGene(const uint16_t connectionID, Genome* genomePtr) {
        const auto sourceID = static_cast<uint8_t>(connectionID >> 8);
        const auto destID = static_cast<uint8_t>(connectionID);
        const uint16_t weight = genomePtr->connections[connectionID];

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
                        genomePtr->biases[newSourceID] = getRandomValue();
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
                        genomePtr->biases[newDestID] = getRandomValue();
                }
                break;
            }
            case 2: { //weight mutation
                genomePtr->connections[connectionID] = getRandomValue();
                break;
            }
                //bias mutation
            case 3: {
                genomePtr->biases[sourceID] = getRandomValue();
                genomePtr->biases[destID] = getRandomValue();
                break;
            }
            case 4: { //mutate whole gene
                const uint16_t newConnectionID = getRandomConnectionID();

                if(!genomePtr->connections.contains(newConnectionID)) {
                    genomePtr->connections.erase(connectionID);
                    genomePtr->connections[newConnectionID] = getRandomValue();
                    const auto newSourceID = static_cast<uint8_t>(newConnectionID >> 8);
                    const auto newDestID = static_cast<uint8_t>(newConnectionID);
                    genomePtr->biases[newSourceID] = getRandomValue();
                    genomePtr->biases[newDestID] = getRandomValue();
                }else {
                    genomePtr->connections[connectionID] = getRandomValue();
                    genomePtr->biases[sourceID] = getRandomValue();
                    genomePtr->biases[destID] = getRandomValue();
                }
                break;
            }
            default: break;
        }
    }

    inline void mutateTraitGenome(TraitGenome* traitGenomePtr) {
        std::bernoulli_distribution distChanceToMutate(0.10);
        for(auto& trait : traitGenomePtr->traits) {
            if(distChanceToMutate(mt)) {
               trait = getRandomValue();
            }
        }
    }

    inline void mutateGenome(Genome* genomePtr) {
        if(genomePtr->connections.empty()) return;

        std::geometric_distribution<int> distNumConnectionsToMutate(0.2);
        std::uniform_int_distribution<int> distConnectionsToMutate(0, static_cast<int>(genomePtr->connections.size()) - 1);
        int numConnectionsToMutate;
        std::set<int> connectionsToMutate;

        do {
            numConnectionsToMutate = distNumConnectionsToMutate(mt);
        } while(numConnectionsToMutate < 1 || numConnectionsToMutate > genomePtr->connections.size());

        for(int i = 0; i < numConnectionsToMutate; i++) {
            connectionsToMutate.insert(distConnectionsToMutate(mt));
        }

        for(const int connectionToMutate : connectionsToMutate) {
            auto itrConnections = genomePtr->connections.begin();
            std::advance(itrConnections, connectionToMutate);
            mutateGene(itrConnections->first, genomePtr);
        }
    }
}
#endif //GENOME_HPP