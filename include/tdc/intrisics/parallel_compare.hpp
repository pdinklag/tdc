#pragma once

#include <cstdint>

namespace tdc {
namespace intrisics {

/// \brief Performs a parallel unsigned byte-wise greater-than comparison of the given 64-bit words.
///
/// The result is a 64-bit word of eight bytes containing the result of each comparison - \c 0xFF if the byte from \c a was greater than that from \c b and \c 0x00 otherwise.
///
/// \param a the first word
/// \param b the second word
uint64_t pcmpgtub(const uint64_t a, const uint64_t b);
    
}} // namespace tdc::intrisics
