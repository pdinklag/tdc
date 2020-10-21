#pragma once

#include <tdc/intrisics/lzcnt.hpp>
#include <tdc/util/likely.hpp>
#include <tdc/util/uint40.hpp>
#include <cstdint>

namespace tdc {
namespace math {

/// \cond INTERNAL
template<typename T>
struct _ilog2;

template<>
struct _ilog2<uint8_t> {
    static constexpr size_t ceil(const uint8_t x) {
        return 64ULL - __builtin_clzll(x);
    }
};

template<>
struct _ilog2<uint16_t> {
    static constexpr size_t ceil(const uint16_t x) {
        return 64ULL - __builtin_clzll(x);
    }
};

template<>
struct _ilog2<uint32_t> {
    static constexpr size_t ceil(const uint32_t x) {
        return 64ULL - __builtin_clzll(x);
    }
};

template<>
struct _ilog2<uint40_t> {
    static constexpr size_t ceil(const uint40_t x) {
        return 64ULL - __builtin_clzll((uint64_t)x);
    }
};

template<>
struct _ilog2<uint64_t> {
    static constexpr size_t ceil(const uint64_t x) {
        return 64ULL - __builtin_clzll(x);
    }
};
/// \endcond

/// \brief Returns the number of bits required to store an integer value at most the given number.
///
/// Please mind that this is not the 2-logarithm in a strict mathematical sense.
/// This function returns zero if the input is zero. For any input <tt>x > 0</tt> it computes the location of the most significant set bit.
///
/// \tparam T the integer type
/// \param x the number in question
template<typename T>
inline constexpr size_t ilog2_ceil(const T x) {
    return _ilog2<T>::ceil(x);
}

/// \brief Returns the number of bits required to store an integer value at most the given number, minus one.
///
/// Please mind that this is not the 2-logarithm in a strict mathematical sense.
/// This function returns zero if the input is zero. For any input <tt>x > 0</tt> it computes the location of the most significant set bit minus one.
///
/// \param x the number in question
template<typename T>
inline constexpr size_t ilog2_floor(const T x) {
    if(tdc_unlikely(x == 0)) return 0;
    return ilog2_ceil(x) - 1ULL;
}

}} // namespace tdc::math
