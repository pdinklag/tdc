#pragma once

#include <tdc/uint/uint_half.hpp>
#include <tdc/uint/uint_pow2.hpp>

#ifdef __SIZEOF_INT128__

namespace tdc {
    
using uint128_t = __uint128_t;

template<>
struct uint_pow2_half<256> {
    using type = uint128_t;
};

/// \cond INTERNAL
namespace intrisics {
template<>
constexpr size_t lzcnt<uint128_t>(const uint128_t& x) {
    const size_t hi = lzcnt0((uint64_t)(x >> 64));
    return hi == 64ULL ? 64ULL + lzcnt0((uint64_t)x) : hi;
};

template<>
constexpr size_t popcnt<uint128_t>(const uint128_t& x) {
    return popcnt((uint64_t)x) + popcnt((uint64_t)(x >> 64));
};

template<>
constexpr size_t tzcnt<uint128_t>(const uint128_t& x) {
    const size_t lo = tzcnt0((uint64_t)x);
    return lo == 64ULL ? 64ULL + tzcnt0((uint64_t)(x >> 64)) : lo;
};

} // namespace intrisics
} // namespace tdc

namespace std {

/// \brief Standard output support  for \ref tdc::uint128_t.
inline ostream& operator<<(ostream& out, tdc::uint128_t v) {
    return tdc::print_uint(out, v);
}

} // namespace std

#else
   
namespace tdc {

using uint128_t = uint_pow2_t<128>;

/// \cond INTERNAL
namespace intrisics {
    template<> constexpr size_t lzcnt<uint128_t>(const uint128_t& x) { return x.lzcnt(); }
    template<> constexpr size_t popcnt<uint128_t>(const uint128_t& x) { return x.popcnt(); }
    template<> constexpr size_t tzcnt<uint128_t>(const uint128_t& x) { return x.tzcnt(); }
} // namespace intrisics
/// \endcond
} // namespace tdc

namespace std {

/// \brief Standard output support for \ref tdc::uint128_t.
inline ostream& operator<<(ostream& out, const tdc::uint128_t& x) { return x.outp(out); }

} // namespace std

#endif

// sanity checks
static_assert(sizeof(tdc::uint128_t) == 16);
static_assert(std::numeric_limits<tdc::uint128_t>::digits == 128);

/// \brief Global definition of 128-bit integers.
using uint128_t = tdc::uint128_t;

/// \brief The maximum value for 128-bit integers.
#define UINT128_MAX std::numeric_limits<uint128_t>::max()

namespace tdc {
    template<> struct uint_half<uint128_t> { using type = uint64_t; };
}
