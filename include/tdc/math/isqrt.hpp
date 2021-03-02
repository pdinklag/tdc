#pragma once

#include <cstdint>

#include <tdc/math/ilog2.hpp>

namespace tdc {
namespace math {

/// \brief Returns the square root of the given number, rounded down to the next smaller integer.
/// \param x the number in question
constexpr uint64_t isqrt_floor(const uint64_t x) {
    if(x < 4ULL) {
        return uint64_t(x > 0);
    }
    
    uint64_t e = ilog2_floor(x) >> 1;
    uint64_t r = 1ULL;

    while(e--) {
        const uint64_t cur = x >> (e << 1ULL);
        const uint64_t sm = r << 1ULL;
        const uint64_t lg = sm + 1ULL;
        r = (sm + (lg * lg <= cur));
    }
    return r;
}

/// \brief Returns the square root of the given number, rounded up to the next greater integer.
/// \param x the number in question
constexpr uint64_t isqrt_ceil(const uint64_t x) {
    const uint64_t r = isqrt_floor(x);
    return r + (r * r < x);
}

}} // namespace tdc::math
