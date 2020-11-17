#pragma once

#include <tdc/uint/uint512.hpp>

namespace tdc {

using uint1024_t = uint_pow2_t<1024>;
static_assert(std::is_same<uint1024_t::half_t, uint512_t>::value);

/// \cond INTERNAL
namespace intrisics {
    template<> constexpr size_t lzcnt<uint1024_t>(const uint1024_t& x) { return x.lzcnt(); }
    template<> constexpr size_t popcnt<uint1024_t>(const uint1024_t& x) { return x.popcnt(); }
    template<> constexpr size_t tzcnt<uint1024_t>(const uint1024_t& x) { return x.tzcnt(); }
} // namespace intrisics
/// \endcond
} // namespace tdc

namespace std {

/// \brief Standard output support for \ref tdc::uint1024_t.
inline ostream& operator<<(ostream& out, const tdc::uint1024_t& x) { return x.outp(out); }

} // namespace std


// sanity checks
static_assert(sizeof(tdc::uint1024_t) == 128);
static_assert(std::numeric_limits<tdc::uint1024_t>::digits == 1024);

/// \brief Globally define 1024-bit integers.
using uint1024_t = tdc::uint1024_t;

/// \brief The maximum value for 1024-bit integers.
#define UINT1024_MAX std::numeric_limits<uint1024_t>::max()

