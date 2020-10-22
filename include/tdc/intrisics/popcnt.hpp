#pragma once

#include <cstddef>
#include <cstdint>
#include <tdc/util/uint40.hpp>

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
/// \endcond

}} // namespace tdc::intrisics
