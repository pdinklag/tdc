#pragma once

#include <bit>
#include <concepts>
#include <limits>
#include <random>
#include <stdexcept>

#include <tdc/uint/uint128.hpp>

namespace tdc {
namespace hash {

class RollingKarpRabinFingerprint {
private:
    static constexpr uint64_t MERSENNE61 = (((uint64_t)1ULL) << 61) - 1;
    static constexpr uint64_t MERSENNE61_SHIFT = 61;
    static constexpr uint128_t MERSENNE61_SQUARE = (uint128_t)MERSENNE61 * MERSENNE61;

    inline static uint128_t mult(uint64_t const a, uint64_t const b) {
        return ((uint128_t)a) * b;
    }

    inline static uint64_t modulo(const uint128_t value) {
        // this assumes value < (2^61)^2 = 2^122
        const uint128_t v = value + 1;
        const uint64_t z = ((v >> MERSENNE61_SHIFT) + v) >> MERSENNE61_SHIFT;
        return (value + z) & MERSENNE61;
    }

    inline static uint64_t random_base() {
        constexpr static uint64_t max = MERSENNE61 - 2;
        static std::random_device seed;
        static std::mt19937_64 g(seed());
        static std::uniform_int_distribution<uint64_t> d(0, max);
        return 1 + d(g); // avoid returning 0
    }

    inline static uint128_t square(const uint64_t a) {
        return ((uint128_t)a) * a;
    }

    inline static uint64_t power(uint64_t base, uint64_t exponent) {
        uint64_t result = 1;
        while(exponent > 0) {
            if(exponent & 1ULL) result = modulo(mult(base, result));
            base = modulo(square(base));
            exponent >>= 1;
        }
        return result;
    }

    uint64_t const base_;
    uint64_t const max_exponent_exclusive_; // base ^ window_size

public:
    RollingKarpRabinFingerprint(const uint64_t window, const uint64_t base) : base_(modulo(base)), max_exponent_exclusive_(power(base_, window)) {
    }

    RollingKarpRabinFingerprint(const uint64_t window) : RollingKarpRabinFingerprint(window, random_base()) {
    }
    
    RollingKarpRabinFingerprint(const RollingKarpRabinFingerprint&) = default;
    RollingKarpRabinFingerprint(RollingKarpRabinFingerprint&&) = default;
    RollingKarpRabinFingerprint& operator=(const RollingKarpRabinFingerprint&) = default;
    RollingKarpRabinFingerprint& operator=(RollingKarpRabinFingerprint&&) = default;
    
    inline uint64_t roll(const uint64_t fp, const uint64_t pop_left, const uint64_t push_right) {
        const uint128_t shifted_fingerprint = mult(base_, fp);
        const uint128_t pop = MERSENNE61_SQUARE - mult(max_exponent_exclusive_, pop_left);
        return modulo(shifted_fingerprint + pop + push_right);
    }
};

}} // namespace tdc::hash
