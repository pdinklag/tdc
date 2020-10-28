#pragma once

#include <tdc/uint/print_uint.hpp>

#ifdef __SIZEOF_INT128__

namespace tdc {
    
using uint128_t = __uint128_t;

}

#else

static_assert(false, "__uint128_t is not available");

#endif

namespace std {

inline ostream& operator<<(ostream& out, tdc::uint128_t v) {
    return tdc::print_uint(out, v);
}

} // namespace std

#include <climits>

/// \brief Globally define 128-bit integers.
using uint128_t = tdc::uint128_t;

/// \brief The maximum value for 128-bit integers.
#define UINT128_MAX std::numeric_limits<uint128_t>::max()
