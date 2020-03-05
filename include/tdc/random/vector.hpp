#pragma once

#include <random>
#include <vector>

#include "seed.hpp"

namespace tdc {
namespace random {

/// \cond INTERNAL

// this is used to map bool to uint8_t, because you cannot have a uniform_int_distribution for the bool type
template<typename T> struct map_type { using value = T; };
template<> struct map_type<bool> { using value = uint8_t; };
/// \endcond

/// \brief Generates a vector of random numbers between 0 and the given maximum (inclusive).
/// \tparam the number type
/// \param num the number of numbers to generate
/// \param max the maximum possible number to be generated
/// \param seed the random seed
template<typename T>
std::vector<T> vector(const size_t num, const T max, const uint64_t seed = DEFAULT_SEED) {
    std::vector<T> v;
    v.reserve(num);

    // seed
    std::default_random_engine gen(seed);
    std::uniform_int_distribution<typename map_type<T>::value> dist(0, max);

    // generate
    for(size_t i = 0; i < num; i++) {
        v.push_back(dist(gen));
    }

    return v;
}

}} // namespace tdc::random
