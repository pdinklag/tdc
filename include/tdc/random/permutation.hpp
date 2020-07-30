#pragma once

#include <random>
#include <vector>

#include "seed.hpp"

namespace tdc {
namespace random {

/// \brief Generates a random permutation of a given universe with near-uniform distribution.
///
/// This is based on an article by Jeff Preshing (https://preshing.com/20121224/how-to-generate-a-sequence-of-unique-random-integers), who describes how to generate random permutations of 32-bit numbers using quadratic residues of primes,
/// modified to support an arbitrary universe size.
class Permutation {
private:
    // finds the largest prime p less than or equal to x that satisfies p = (3 mod 4)
    static uint64_t prev_prime_3mod4(const uint64_t x);

    uint64_t m_universe;
    uint64_t m_seed;
    uint64_t m_prime;

    // permute the given number
    uint64_t permute(const uint64_t x) const;

public:
    /// \brief Default constructor.
    ///
    /// The default permutation contains only zero.
    Permutation();
    
    Permutation(const Permutation&) = default;
    Permutation(Permutation&&) = default;
    Permutation& operator=(const Permutation&) = default;
    Permutation& operator=(Permutation&&) = default;

    /// \brief Initializes a permutation.
    /// \param universe the universe size
    /// \param seed the random seed
    Permutation(const uint64_t universe, const uint64_t seed = DEFAULT_SEED);

    /// \brief Permutes the given number.
    /// \param i the number to permute, must be within the universe
    uint64_t operator()(const uint64_t i) const;

    /// \brief Permutes a set of increasing numbers and returns the permuted numbers as a vector.
    /// \param num the number of numbers to permute
    /// \param start the first number to permute
    std::vector<uint64_t> vector(const size_t num, const uint64_t start = 0ULL) const;
};

}} // namespace tdc::random
