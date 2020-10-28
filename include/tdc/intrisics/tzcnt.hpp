#pragma once

#include <cstddef>
#include <cstdint>
#include <tdc/math/iminmax.hpp>
#include <tdc/uint/uint40.hpp>
#include <tdc/uint/uint128.hpp>
#include <tdc/uint/uint256.hpp>

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
    return math::imin((size_t)__builtin_ctz(x), size_t(8)); // there can be at most 8 trailing zeroes
};

template<>
constexpr size_t tzcnt(const uint16_t x) {
    return math::imin((size_t)__builtin_ctz(x), size_t(16)); // there can be at most 16 trailing zeroes
};

template<>
constexpr size_t tzcnt(const uint32_t x) {
    return __builtin_ctz(x);
};

template<>
constexpr size_t tzcnt(const uint40_t x) {
    return math::imin((size_t)__builtin_ctzll((uint64_t)x), size_t(40));  // there can be at most 40 trailing zeroes
};

template<>
constexpr size_t tzcnt(const uint64_t x) {
    return __builtin_ctzll(x);
};

template<>
constexpr size_t tzcnt(const uint128_t x) {
    const size_t lo = tzcnt((uint64_t)x);
    return lo == 64ULL ? 64ULL + tzcnt((uint64_t)(x >> 64)) : lo;
};

template<>
constexpr size_t tzcnt(const uint256_t x) {
    const size_t lo = tzcnt((uint128_t)x);
    return lo == 128ULL ? 128ULL + tzcnt((uint128_t)(x >> 128)) : lo;
};
/// \endcond

}} // namespace tdc::intrisics

