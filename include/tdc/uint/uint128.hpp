#pragma once

#include <tdc/uint/print_uint.hpp>

#ifdef __SIZEOF_INT128__

namespace tdc {
    
using uint128_t = __uint128_t;

#else

static_assert(false, "__uint128_t is not available");

#endif

/// \cond INTERNAL
namespace intrisics {
    
template<>
constexpr size_t lzcnt(const uint128_t x) {
    const size_t hi = lzcnt0((uint64_t)(x >> 64));
    return hi == 64ULL ? 64ULL + lzcnt0((uint64_t)x) : hi;
};

template<>
constexpr size_t popcnt(const uint128_t x) {
    return popcnt((uint64_t)x) + popcnt((uint64_t)(x >> 64));
};

template<>
constexpr size_t tzcnt(const uint128_t x) {
    const size_t lo = tzcnt0((uint64_t)x);
    return lo == 64ULL ? 64ULL + tzcnt0((uint64_t)(x >> 64)) : lo;
};

} // namespace intrisics
/// \endcond INTERNAL
} // namespace tdc

namespace std {

/// \brief Standard output support  for \ref tdc::uint128_t.
inline ostream& operator<<(ostream& out, tdc::uint128_t v) {
    return tdc::print_uint(out, v);
}

} // namespace std

#include <climits>

/// \brief Globally define 128-bit integers.
using uint128_t = tdc::uint128_t;

/// \brief The maximum value for 128-bit integers.
#define UINT128_MAX std::numeric_limits<uint128_t>::max()
