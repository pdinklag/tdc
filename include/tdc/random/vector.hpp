#pragma once

#include <vector>
#include "seed.hpp"

namespace tdc {
namespace random {

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
    std::uniform_int_distribution<T> dist(0, max);

    // generate
    for(size_t i = 0; i < num; i++) {
        v.push_back(dist(gen));
    }

    return v;
}

}} // namespace tdc::random
