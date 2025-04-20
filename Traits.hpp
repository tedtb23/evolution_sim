#ifndef TRAITS_HPP
#define TRAITS_HPP

#define TRAITS_VALUES(...) constexpr char const* traitsStrValues[] = {__VA_ARGS__}
#define TRAITS_SIZE (sizeof(traitsStrValues) / sizeof(traitsStrValues[0]))

enum Traits {
    GROWTH,
    SPEED,
    FERTILITY,
    OXYGEN_ATMOSPHERE,
    HYDROGEN_ATMOSPHERE,
    AGGRESSION,
    HEAT_TOLERANCE,
    COLD_TOLERANCE,
};

TRAITS_VALUES("GROWTH",
              "SPEED",
              "FERTILITY",
              "OXYGEN_ATMOSPHERE",
              "HYDROGEN_ATMOSPHERE",
              "AGGRESSION",
              "HEAT_TOLERANCE",
              "COLD_TOLERANCE",);

#endif //TRAITS_HPP
