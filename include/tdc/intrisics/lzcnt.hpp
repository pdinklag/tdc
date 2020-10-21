#pragma once

#include <cstddef>
#include <cstdint>

namespace tdc {
namespace intrisics {

/// \brief Counts the number of leading zero bits in an integer.
/// \tparam T the integer type.
template<typename T>
constexpr size_t lzcnt(T x);

/// \brief Counts the number of leading zero bits in a 32-bit integer.
template<>
constexpr size_t lzcnt<uint32_t>(uint32_t x) {
    return __builtin_clz(x);
};

/// \brief Counts the number of leading zero bits in a 64-bit integer.
template<>
constexpr size_t lzcnt<uint64_t>(uint64_t x) {
        return __builtin_clzll(x);
};

}} // namespace tdc::intrisics

