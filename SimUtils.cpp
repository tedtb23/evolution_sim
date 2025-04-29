#include "SimUtils.hpp"

#include <random>

namespace SimUtils {
    std::mt19937 mt{std::random_device{}()};
}