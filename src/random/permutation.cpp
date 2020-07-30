#include <tdc/random/permutation.hpp>
#include <tdc/math/prime.hpp>

using namespace tdc::random;

// some known universe sizes and the corresponding primes that satisfy 3mod4
struct known_universe_t {
    uint64_t universe, prime;
};

constexpr uint64_t pow2(const uint64_t n) {
    return 1ULL << n;
}

known_universe_t known_universes[] = {
    { pow2(16) - 2, pow2(16) - 17 },
    { pow2(16) - 1, pow2(16) - 17 },
    { pow2(24) - 2, pow2(24) - 17 },
    { pow2(24) - 1, pow2(24) - 17 },
    { pow2(32) - 2, pow2(32) - 5 },
    { pow2(32) - 1, pow2(32) - 5 },
    { pow2(40) - 2, pow2(40) - 213 },
    { pow2(40) - 1, pow2(40) - 213 },
    { pow2(48) - 2, pow2(48) - 65 },
    { pow2(48) - 1, pow2(48) - 65 },
    { pow2(56) - 2, pow2(56) - 5 },
    { pow2(56) - 1, pow2(56) - 5 },
    { pow2(63) - 2, pow2(63) - 25 },
    { pow2(63) - 1, pow2(63) - 25 },
    { UINT64_MAX-1, 0xFFFFFFFFFFFFFF43ULL },
    { UINT64_MAX,   0xFFFFFFFFFFFFFF43ULL },
    { 0, 0 }
};

// provides a decent distribution of 64 bits
constexpr uint64_t SHUFFLE1 = 0x9696594B6A5936B2ULL;
constexpr uint64_t SHUFFLE2 = 0xD2165B4B66592AD6ULL;

uint64_t Permutation::prev_prime_3mod4(const uint64_t universe) {
    // test if universe is known
    for(size_t i = 0; known_universes[i].prime > 0; i++) {
        if(universe == known_universes[i].universe) {
            return known_universes[i].prime;
        }
    }
    
    // otherwise, do it the hard way
    uint64_t p = math::prime_predecessor(universe);
    while((p & 3ULL) != 3ULL) {
        p = math::prime_predecessor(p - 1);
    }
    return p;
}

uint64_t Permutation::permute(const uint64_t x) const {
    if(x >= m_prime) {
        // map numbers in gap to themselves - shuffling will take care of this
        return x;
    } else {
        // use quadratic residue
        const uint64_t r = ((__uint128_t)x * (__uint128_t)x) % (__uint128_t)m_prime;
        return (x <= (m_prime >> 1ULL)) ? r : m_prime - r;
    }
}

Permutation::Permutation() : m_universe(1), m_seed(0), m_prime(0) {
}

Permutation::Permutation(const uint64_t universe, const uint64_t seed)
    : m_universe(universe),
      m_prime(prev_prime_3mod4(universe)),
      m_seed((seed ^ SHUFFLE1) ^ SHUFFLE2) {
}

uint64_t Permutation::operator()(const uint64_t i) const {
    return permute((m_seed + permute(i)) % m_universe);
}

std::vector<uint64_t> Permutation::vector(const size_t num, const uint64_t start) const {
    std::vector<uint64_t> vec(num);
    for(size_t i = 0; i < num; i++) {
        vec[i] = (*this)(start + i);
    }
    return vec;
}
