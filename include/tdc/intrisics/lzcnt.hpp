#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace tdc {
namespace intrisics {

/// \brief Counts the number of leading zero bits in an integer.
///
/// Note that the output is \em undefined if the input is zero. Use \ref lzcnt0 if the input may be zero.
/// \tparam T the integer type
/// \param x the integer
template<typename T>
constexpr size_t lzcnt(const T x);

/// \brief Counts the number of leading zero bits in an integer.
///
/// This version accepts zero as an input, but at the cost of an additional branch. Use \ref lzcnt directly if the input cannot be zero.
/// \tparam T the integer type
/// \param x the integer
template<typename T>
constexpr size_t lzcnt0(const T x) {
    return x == T(0) ? std::numeric_limits<T>::digits : lzcnt(x);
}

/// \cond INTERNAL
template<>
constexpr size_t lzcnt(const uint8_t x) {
    assert(x > 0);
    return __builtin_clz(x) - 24ULL; // don't account for 24 padded bits
};

template<>
constexpr size_t lzcnt(const uint16_t x) {
    assert(x > 0);
    return __builtin_clz(x) - 16ULL; // don't account for 16 padded bits
};

template<>
constexpr size_t lzcnt(const uint32_t x) {
    assert(x > 0);
    return __builtin_clz(x);
};

template<>
constexpr size_t lzcnt(const uint64_t x) {
    assert(x > 0);
    return __builtin_clzll(x);
};

/// \endcond

}} // namespace tdc::intrisics

