#pragma once

#include <concepts>
#include <cstdint>
#include <limits>
#include <tdc/util/likely.hpp>

namespace tdc {
namespace math {

/// \brief Gets a bit mask for the specified number of low bits.
///
/// More specifically, this computes <tt>2^bits-1</tt>, the value where the lowest \c bits bits are set.
///
/// To retrieve a bit mask for high bits, simply compute the bit negation of the result, ie., <tt>~bit_mask(bits)</tt>.
///
/// \tparam T the type of the bit mask
/// \param bits the number of low bits to mask, must be greater than zero
template<std::unsigned_integral T>
constexpr T bit_mask(const size_t bits) {
    if(tdc_unlikely(bits >= std::numeric_limits<T>::digits)) return std::numeric_limits<T>::max();
    return ~(std::numeric_limits<T>::max() << bits);
}

}} // namespace tdc::math
