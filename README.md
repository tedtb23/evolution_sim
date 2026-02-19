# Evolution Simulation
Simulates the process of evolution through natural selection with organisms that have their behavior driven by neural network brains. Inspired by David Miller's biosim4 project https://github.com/davidrmiller/biosim4.

<img width="1279" height="720" alt="image" src="https://github.com/user-attachments/assets/92fda660-461b-460f-8188-5921ae2400f6" />


## Installation
 ### Prerequisites
 - git >= 2.39.5
 - cmake >= 3.31.3
 - MacOS (Tested on Sequoia 15.5) OR
 - Windows 11 (Tested on 24H2) OR
 - Linux (Tested on Ubuntu 25.10) 
 - a c++ compiler
   
1. Open a terminal to the directory you want evolution_sim installed to
2. Clone the repository `git clone --recurse-submodules https://github.com/tedtb23/evolution_sim`
3. Navigate to the evolution_sim directory in the terminal `cd PATH_TO_GIT_CLONE_DIR/evolution_sim`
4. Make a build directory `mkdir build`
5. Navigate to the newly created build directory `cd build`
6. Use cmake to configure the project for building `cmake ..`
7. Use cmake to build the project `cmake --build .`

## Simulation overview
The simulation consists of a 2D plane filled with organisms.  
This plane is filled with a limited amount of food that will replinish an organism's hunger. 
Food respawns at the start of every generation.
The user can change the food spawn region or press ENTER to make the food spawn region randomly change every few seconds. Once the food spawn region is being randomly changed, the user can press ESC to go back to setting the food spawn region themselves.  
The plane also consists of a temperature and atmosphere map which give different regions of the plane different temperatures and atmospheres.
The user can click to view the temperature and atmosphere maps.  
The user can click to randomize the spawn of organisms across the plane, or deselect it to have organisms spawn close to one of their parents.  
The user can left-click on any organism in the simulation to see statistics about that organism. Including the current state of their neural net brain, their hunger, etc.  
The user can middle-click on any organism to see that organism's traits.  
Organisms have a neural network genome and a trait genome (discussed further in the Genome overview section) that determines an organism's neural network structure and trait composition.
These genomes can be inherited and mutate over time.  
Organisms can emit pheromones around dangers like fire to alert other organisms to its presence.  
#### There are several selection criteria that will determine which organisms survive to reproduce and which ones don't.
- Organisms must eat food to replenish their hunger, breathe in their appropriate atmosphere (as determined by their traits), and avoid obstacles to survive.
- Organisms that eat a certain amount of food in their lifetime will be eligable to reproduce and will be randomly matched with another organism to reproduce with at the end of the generation.
- Organisms that don't eat food and let their hunger reach 0 will die. 
- Randomly placed fires will spawn around the plane and any organisms that touch them will die.
- Organisms that leave the simulation bounds (move off screen) will die.
- Organisms with a low tolerance to their current temperature will have their acceleration reduced and hunger gain rate increased (i.e. an organism in a cold region of the map with a low cold tolerance trait will move more slowly and lose their hunger more quickly).
- Organisms with a low ability to breathe their current atmosphere will gradually lose their breathe and die (i.e. an organism with a high hydrogen atmosphere trait and a low oxygen atmosphere trait will gradually lose their breathe in an oxygen atmosphere and die).

## Organism overview
Organisms have a hunger meter and breath meter.  
Organisms have a neural network and a trait genome.  
An organism's traits determine their characteristics such as their ability to tolerate cold temperatures, the natural size they will grow to, or their natural speed of movement across the plane.  
An organism's neural network consists of input neurons, which receive activations according to the current state of the organism in the simulation, a hidden layer of "thinking" neurons, and output neurons, whose calculated activations indicate actions the organism should take into the next frame of the simulation (i.e. move left and eat food).  
Organisms' ultimate goal are to survive long enough to reproduce and carry their genes onto their children. Although there is no explicit programming telling them this is their goal. 
Organisms will start out in the first generation of the simulation with completely random neural networks and traits, and as such their observed behavior will be completely random. However, over time organisms with useless neural networks and traits will die off whilst organisms that were randomly given useful neural networks and traits (or mutated into them) will live on to reproduce and pass down their genes.  
After several generations logical behavior can be observed in the organisms, such as moving towards food or away from dangers like fire.

## Genome overview
An organism's neural network brain structure is determined by their genome, which can be inherited from their parents or completely randomly generated (which only happens in the first generation as there are no genomes to inherit from at the start).  
Organisms also have a trait genome that determines how active certain traits like cold tolerance or ability to breathe an oxygen atmosphere are. The traits genome can also be inherited the same as their neural network genome can.  
Both of these genomes in every organism have a random chance to mutate every frame of the simulation. Allowing for natural variation in the gene pool as generations go on.  
Genomes inherited from 2 parents are a random composition of the 2 parents genes.

## List of possible neurons
### Input Neurons
- HUNGER
- FOOD_LEFT
- FOOD_RIGHT
- FOOD_UP
- FOOD_DOWN
- FOOD_COLLISION
- ORGANISM_LEFT
- ORGANISM_RIGHT
- ORGANISM_UP
- ORGANISM_DOWN
- ORGANISM_COLLISION
- FIRE_LEFT
- FIRE_RIGHT
- FIRE_UP
- FIRE_DOWN
- DETECT_DANGER_PHEROMONE
- BOUNDS_LEFT
- BOUNDS_RIGHT
- BOUNDS_UP
- BOUNDS_DOWN
- TEMPERATURE
- OXYGEN_SATURATION
- HYDROGEN_SATURATION

### Output Neurons
- MOVE_LEFT
- MOVE_RIGHT
- MOVE_UP
- MOVE_DOWN
- EAT

## List of traits
- GROWTH
- SPEED
- FERTILITY
- OXYGEN_ATMOSPHERE
- HYDROGEN_ATMOSPHERE
- AGGRESSION (currently unused)
- HEAT_TOLERANCE
- COLD_TOLERANCE
