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

/// \brief Returns the number of bits required to store an integer value at most the given number.
///
/// Please mind that this is not the 2-logarithm in a strict mathematical sense.
/// This function returns zero if the input is zero. For any input <tt>x > 0</tt> it computes the location of the most significant set bit.
///
/// \param x the number in question
inline constexpr uint64_t ilog2_ceil(const uint64_t x) {
    return 64ULL - __builtin_clzll(x);
}

}}
