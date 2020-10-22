#pragma once

#include <cstddef>
#include <cstdint>
#include <tdc/util/uint40.hpp>

namespace tdc {
namespace intrisics {

/// \brief Counts the number of trailing zero bits in an integer.
/// \tparam T the integer type
/// \param x the integer
template<typename T>
constexpr size_t tzcnt(const T x);

/// \cond INTERNAL
template<>
constexpr size_t tzcnt(const uint8_t x) {
    return __builtin_ctz(x) - 24ULL;
};

template<>
constexpr size_t tzcnt(const uint16_t x) {
    return __builtin_ctz(x) - 16ULL;
};

template<>
constexpr size_t tzcnt(const uint32_t x) {
    return __builtin_ctz(x);
};

template<>
constexpr size_t tzcnt(const uint40_t x) {
        return __builtin_ctzll((uint64_t)x) - 24ULL;
};

template<>
constexpr size_t tzcnt(const uint64_t x) {
        return __builtin_ctzll(x);
};
/// \endcond

}} // namespace tdc::intrisics

