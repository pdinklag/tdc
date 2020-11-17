#pragma once

#include <tdc/uint/uint256.hpp>

// TODO: use AVX512 registers if available

namespace tdc {

using uint512_t = uint_pow2_t<512>;
static_assert(std::is_same<uint512_t::half_t, uint256_t>::value);

/// \cond INTERNAL
namespace intrisics {
    template<> constexpr size_t lzcnt<uint512_t>(const uint512_t& x) { return x.lzcnt(); }
    template<> constexpr size_t popcnt<uint512_t>(const uint512_t& x) { return x.popcnt(); }
    template<> constexpr size_t tzcnt<uint512_t>(const uint512_t& x) { return x.tzcnt(); }
} // namespace intrisics
/// \endcond
} // namespace tdc

namespace std {

/// \brief Standard output support for \ref tdc::uint512_t.
inline ostream& operator<<(ostream& out, const tdc::uint512_t& x) { return x.outp(out); }

} // namespace std


// sanity checks
static_assert(sizeof(tdc::uint512_t) == 64);
static_assert(std::numeric_limits<tdc::uint512_t>::digits == 512);

/// \brief Globally define 512-bit integers.
using uint512_t = tdc::uint512_t;

/// \brief The maximum value for 512-bit integers.
#define UINT512_MAX std::numeric_limits<uint512_t>::max()

