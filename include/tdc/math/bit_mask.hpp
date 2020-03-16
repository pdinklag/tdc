#pragma once

#include <cstdint>

namespace tdc {
namespace math {

/// \brief Gets a bit mask for the specified number of low bits.
///
/// More specifically, this computes <tt>2^bits-1</tt>, the value where the lowest \c bits bits are set.
/// Note that \c bits must be greater than zero; the result for <tt>bits=0</tt> is undefined.
///
/// To retrieve a bit mask for high bits, simply compute the bit negation of the result, ie., <tt>~bit_mask(bits)</tt>.
///
/// \param bits the number of low bits to mask, must be greater than zero
inline constexpr uint64_t bit_mask(const uint64_t bits) {
    return UINT64_MAX >> (64ULL - bits);
}

}} // namespace tdc::math
