#pragma once

#include <cstdint>

namespace tdc {
namespace math {

/// \brief Computes the rounded-up integer quotient of two numbers.
/// \param a the dividend
/// \param b the divisor
inline constexpr uint64_t idiv_ceil(const uint64_t a, const uint64_t b) {
    return (a + b - 1ULL) / b;
}

}} // namespace tdc::math
