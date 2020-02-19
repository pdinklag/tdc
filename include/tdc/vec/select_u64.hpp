#pragma once

namespace tdc {
namespace vec {

/// \brief Returned by \ref select0_u64 and \ref select1_u64 in case the searched bit does not exist in the given input value.
constexpr uint8_t SELECT_U64_FAIL = 0xFF;

/// \brief Finds the position of the k-th set bit in a 64-bit word.
///
/// The search starts with the least significant bit.
///
/// \param v the input value
/// \param k the searched 1-bit
/// \return the position of the k-th 1-bit (LSBF and zero-based), or \ref SELECT_U64_FAIL if no such bit exists
inline constexpr uint8_t select1_u64(uint64_t v, uint8_t k) {
    uint8_t pos = 0;
    while(v && k && pos < 64) {
        const size_t z = __builtin_ctzll(v)+1;
        pos += z;
        v >>= z;
        --k;
    }
    return k ? SELECT_U64_FAIL : pos - 1;
}

/// \brief Finds the position of the k-th set a 64-bit word, starting from a given position.
///
/// \param v the input value
/// \param l the bit (LSBF order) to start searching from
/// \param k the searched 1-bit
/// \return the position of the k-th 1-bit (LSBF and zero-based), or \ref SELECT_U64_FAIL if no such bit exists
inline constexpr uint8_t select1_u64(const uint64_t v, const uint8_t l, const uint8_t k) {
    const uint8_t pos = select1_u64(v >> l, k);
    return (pos != SELECT_U64_FAIL) ? (l + pos) : SELECT_U64_FAIL;
}

/// \brief Finds the position of the k-th unset bit in a 64-bit word.
///
/// The search starts with the least significant bit.
///
/// \param v the input value
/// \param k the searched 0-bit
/// \return the position of the k-th 0-bit (LSBF and zero-based), or \ref SELECT_U64_FAIL if no such bit exists
inline constexpr uint8_t select0_u64(const uint64_t v, const uint8_t k) {
    return select1_u64(~v, k);
}

/// \brief Finds the position of the k-th unset a 64-bit word, starting from a given position.
///
/// \param v the input value
/// \param l the bit (LSBF order) to start searching from
/// \param k the searched 0-bit
/// \return the position of the k-th 0-bit (LSBF and zero-based), or \ref SELECT_U64_FAIL if no such bit exists
inline constexpr uint8_t select0_u64(const uint64_t v, const uint8_t l, const uint8_t k) {
    return select1_u64(~v, l, k);
}


}} // namespace tdc::vec
