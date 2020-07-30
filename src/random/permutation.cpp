#include <tdc/random/permutation.hpp>
#include <tdc/math/prime.hpp>

using namespace tdc::random;

// provides a decent distribution of 64 bits
constexpr uint64_t SHUFFLE1 = 0x9696594B6A5936B2ULL;
constexpr uint64_t SHUFFLE2 = 0xD2165B4B66592AD6ULL;

uint64_t Permutation::prev_prime_3mod4(const uint64_t x) {
    uint64_t p = math::prime_predecessor(x);
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
