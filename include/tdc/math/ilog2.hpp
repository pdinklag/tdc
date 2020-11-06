#pragma once

#include <tdc/intrisics/lzcnt.hpp>
#include <tdc/util/int_type_traits.hpp>
#include <tdc/util/likely.hpp>
#include <cstdint>

namespace tdc {
namespace math {

/// \brief Returns the number of bits required to store an integer value at most the given number.
///
/// Please mind that this is not the 2-logarithm in a strict mathematical sense.
/// This function returns zero if the input is zero. For any input <tt>x > 0</tt> it computes the location of the most significant set bit.
///
/// \tparam T the integer type
/// \param x the number in question
template<typename T>
constexpr size_t ilog2_ceil(const T x) {
    return int_type_traits<T>::num_bits() - intrisics::lzcnt0(x);
}
/// \endcond

/// \brief Returns the number of bits required to store an integer value at most the given number, minus one.
///
/// Please mind that this is not the 2-logarithm in a strict mathematical sense.
/// This function returns zero if the input is zero. For any input <tt>x > 0</tt> it computes the location of the most significant set bit minus one.
///
/// \param x the number in question
template<typename T>
constexpr size_t ilog2_floor(const T x) {
    if(tdc_unlikely(x == 0)) return 0;
    return ilog2_ceil(x) - 1ULL;
}

}} // namespace tdc::math
