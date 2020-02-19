#pragma once

#include <cstdint>

namespace tdc {
namespace vec {

/// \cond INTERNAL
inline constexpr uint64_t idiv_ceil(const uint64_t a, const uint64_t b) {
    return (a + b - 1ULL) / b;
}

inline constexpr uint64_t div64_ceil(const uint64_t num) {
    return (num + 63ULL) >> 6ULL;
}

inline constexpr uint64_t log2_ceil(const uint64_t x) {
    return 64ULL - __builtin_clzll(x);
}
/// \endcond

}} // namespace tdc::vec
