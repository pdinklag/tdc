#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <tdc/math/iminmax.hpp>

namespace tdc {
namespace intrisics {

/// \brief Counts the number of trailing zero bits in an integer.
///
/// Note that the output is \em undefined if the input is zero. Use \ref tzcnt0 if the input may be zero.
/// \tparam T the integer type
/// \param x the integer
template<typename T>
constexpr size_t tzcnt(const T& x);

/// \brief Counts the number of trailing zero bits in an integer.
///
/// This version accepts zero as an input, but at the cost of an additional branch. Use \ref tzcnt directly if the input cannot be zero.
/// \tparam T the integer type
/// \param x the integer
template<typename T>
constexpr size_t tzcnt0(const T& x) {
    return x == T(0) ? std::numeric_limits<T>::digits : tzcnt(x);
}

/// \cond INTERNAL
template<>
constexpr size_t tzcnt(const uint8_t& x) {
    assert(x > 0);
    return math::imin((size_t)__builtin_ctz(x), size_t(8)); // there can be at most 8 trailing zeroes
}

template<>
constexpr size_t tzcnt(const uint16_t& x) {
    assert(x > 0);
    return math::imin((size_t)__builtin_ctz(x), size_t(16)); // there can be at most 16 trailing zeroes
}

template<>
constexpr size_t tzcnt(const uint32_t& x) {
    assert(x > 0);
    return __builtin_ctz(x);
}

template<>
constexpr size_t tzcnt(const uint64_t& x) {
    assert(x > 0);
    return __builtin_ctzll(x);
}
/// \endcond

}} // namespace tdc::intrisics

