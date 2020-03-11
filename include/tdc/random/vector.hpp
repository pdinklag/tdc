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
/// \tparam item_t the number type
/// \tparam vector_t the vector type
/// \param num the number of numbers to generate
/// \param max the maximum possible number to be generated
/// \param seed the random seed
template<typename item_t, typename vector_t = std::vector<item_t>>
vector_t vector(const size_t num, const item_t max, const uint64_t seed = DEFAULT_SEED) {
    vector_t v(num);

    // seed
    std::default_random_engine gen(seed);
    std::uniform_int_distribution<typename map_type<item_t>::value> dist(0, max);

    // generate
    for(size_t i = 0; i < num; i++) {
        v[i] = (item_t)dist(gen);
    }

    return v;
}

}} // namespace tdc::random
