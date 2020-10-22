#pragma once

#include <cstddef>
#include <cstdint>
#include <tdc/util/uint40.hpp>

namespace tdc {
namespace intrisics {

/// \brief Counts the number of leading zero bits in an integer.
/// \tparam T the integer type
/// \param x the integer
template<typename T>
constexpr size_t lzcnt(const T x);

/// \cond INTERNAL
template<>
constexpr size_t lzcnt(const uint32_t x) {
    return __builtin_clz(x);
};

template<>
constexpr size_t lzcnt(const uint40_t x) {
        return __builtin_clzll((uint64_t)x);
};

template<>
constexpr size_t lzcnt(const uint64_t x) {
        return __builtin_clzll(x);
};
/// \endcond

}} // namespace tdc::intrisics

