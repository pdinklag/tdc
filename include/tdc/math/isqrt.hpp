#pragma once

#include <cstdint>

namespace tdc {
namespace math {

/// \brief Returns the square root of the given number, rounded down to the next smaller integer.
/// \param x the number in question
uint64_t isqrt_floor(const uint64_t x);

/// \brief Returns the square root of the given number, rounded up to the next greater integer.
/// \param x the number in question
uint64_t isqrt_ceil(const uint64_t x);

}} // namespace tdc::math
