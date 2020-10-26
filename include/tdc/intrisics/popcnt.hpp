#pragma once

#include <cstddef>
#include <cstdint>
#include <tdc/util/uint40.hpp>
#include <tdc/util/uint128.hpp>
#include <tdc/util/uint256.hpp>

namespace tdc {
namespace intrisics {

/// \brief Counts the number of set bits in an integer.
/// \tparam T the integer type
/// \param x the integer
template<typename T>
constexpr size_t popcnt(const T x);

/// \cond INTERNAL
template<>
constexpr size_t popcnt(const uint32_t x) {
    return __builtin_popcount(x);
};

template<>
constexpr size_t popcnt(const uint40_t x) {
    return __builtin_popcountll((uint64_t)x);
};

template<>
constexpr size_t popcnt(const uint64_t x) {
    return __builtin_popcountll(x);
};

template<>
constexpr size_t popcnt(const uint128_t x) {
    return __builtin_popcountll(x) + __builtin_popcountll(x >> 64);
};

template<>
constexpr size_t popcnt(const uint256_t x) {
    return popcnt((uint128_t)x) + popcnt((uint128_t)(x >> 128));
};
/// \endcond

}} // namespace tdc::intrisics
