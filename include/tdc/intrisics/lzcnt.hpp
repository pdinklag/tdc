#pragma once

#include <cstddef>
#include <cstdint>
#include <tdc/util/uint40.hpp>
#include <tdc/util/uint128.hpp>
#include <tdc/util/uint256.hpp>

namespace tdc {
namespace intrisics {

/// \brief Counts the number of leading zero bits in an integer.
/// \tparam T the integer type
/// \param x the integer
template<typename T>
constexpr size_t lzcnt(const T x);

/// \cond INTERNAL
template<>
constexpr size_t lzcnt(const uint8_t x) {
    return __builtin_ctz(x) - 24ULL; // don't account for 24 padded bits
};

template<>
constexpr size_t lzcnt(const uint16_t x) {
    return __builtin_ctz(x) - 16ULL; // don't account for 16 padded bits
};

template<>
constexpr size_t lzcnt(const uint32_t x) {
    return __builtin_clz(x);
};

template<>
constexpr size_t lzcnt(const uint40_t x) {
    return __builtin_clzll((uint64_t)x) - 24ULL; // don't account for 24 padded bits
};

template<>
constexpr size_t lzcnt(const uint64_t x) {
    return __builtin_clzll(x);
};

template<>
constexpr size_t lzcnt(const uint128_t x) {
    const size_t hi = __builtin_clzll(x >> 64);
    return hi == 64ULL ? 64ULL + __builtin_clzll(x) : hi;
};

template<>
constexpr size_t lzcnt(const uint256_t x) {
    const size_t hi = lzcnt((uint128_t)(x >> 128));
    return hi == 128ULL ? 128ULL + lzcnt((uint128_t)x) : hi;
};
/// \endcond

}} // namespace tdc::intrisics

