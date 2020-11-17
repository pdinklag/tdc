#pragma once

#include <tdc/uint/uint128.hpp>

// TODO: use AVX512 registers if available

namespace tdc {

using uint256_t = uint_pow2_t<256>;
static_assert(std::is_same<uint256_t::half_t, uint128_t>::value);

/// \cond INTERNAL
namespace intrisics {
    template<> constexpr size_t lzcnt<uint256_t>(const uint256_t& x) { return x.lzcnt(); }
    template<> constexpr size_t popcnt<uint256_t>(const uint256_t& x) { return x.popcnt(); }
    template<> constexpr size_t tzcnt<uint256_t>(const uint256_t& x) { return x.tzcnt(); }
} // namespace intrisics
/// \endcond
} // namespace tdc

namespace std {

/// \brief Standard output support for \ref tdc::uint256_t.
inline ostream& operator<<(ostream& out, const tdc::uint256_t& x) { return x.outp(out); }

} // namespace std


// sanity checks
static_assert(sizeof(tdc::uint256_t) == 32);
static_assert(std::numeric_limits<tdc::uint256_t>::digits == 256);

/// \brief Globally define 256-bit integers.
using uint256_t = tdc::uint256_t;

/// \brief The maximum value for 256-bit integers.
#define UINT256_MAX std::numeric_limits<uint256_t>::max()

