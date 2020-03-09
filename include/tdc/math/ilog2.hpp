#pragma once

#include <tdc/util/likely.hpp>
#include <cstdint>

namespace tdc {
namespace math {

/// \brief Returns the number of bits required to store an integer value at most the given number.
///
/// Please mind that this is not the 2-logarithm in a strict mathematical sense.
/// This function returns zero if the input is zero. For any input <tt>x > 0</tt> it computes the location of the most significant set bit.
///
/// \param x the number in question
inline constexpr uint64_t ilog2_ceil(const uint64_t x) {
    return 64ULL - __builtin_clzll(x);
}

/// \brief Returns the number of bits required to store an integer value at most the given number, minus one.
///
/// Please mind that this is not the 2-logarithm in a strict mathematical sense.
/// This function returns zero if the input is zero. For any input <tt>x > 0</tt> it computes the location of the most significant set bit minus one.
///
/// \param x the number in question
inline constexpr uint64_t ilog2_floor(const uint64_t x) {
    if(tdc_unlikely(x == 0)) return 0;
    return 64ULL - __builtin_clzll(x) - 1ULL;
}

}} // namespace tdc::math
