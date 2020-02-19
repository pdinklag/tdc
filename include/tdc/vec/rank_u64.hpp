#pragma once

#include <cstdint>

namespace tdc {
namespace vec {

/// \brief Counts the number of set bits in a 64-bit word.
/// \param v the word in question
inline constexpr uint8_t rank1_u64(const uint64_t v) {
    return __builtin_popcountll(v);
}

/// \brief Counts the number of set bits in a 64-bit word up to a given position.
/// \param v the word in question
/// \param x the x-least significant bit up to which to count
inline constexpr uint8_t rank1_u64(const uint64_t v, const uint8_t x) {
    return __builtin_popcountll(v & (UINT64_MAX >> (~x & 63ULL))); // 63 - x
}

/// \brief Counts the number of set bits in a 64-bit word between two a given position a < b.
/// \param v the word in question
/// \param a the a-least significant bit from which to start counting
/// \param b the b-least significant bit up to which to count
inline constexpr uint8_t rank1_u64(const uint64_t v, const uint8_t a, const uint8_t b) {
    const uint64_t mask_a = UINT64_MAX << a;
    const uint64_t mask_b = UINT64_MAX >> (~b & 63ULL); // 63 - b
    return __builtin_popcountll(v & mask_a & mask_b);
}

}} // namespace tdc::vec
