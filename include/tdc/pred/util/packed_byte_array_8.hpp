#pragma once

#include <cstdint>

namespace tdc {
namespace pred {

/// \brief An array of eight bytes packed into a 64-bit word.
union PackedByteArray8 {
    /// \brief The stored bytes.
    uint8_t  u8[8];

    /// \brief The packed 64-bit word.
    uint64_t u64;

    /// \brief Default constructor.
    inline PackedByteArray8() {
    }

    /// \brief Conversion.
    inline PackedByteArray8(uint64_t word) : u64(word) {
    }
};

// sanity check
static_assert(sizeof(PackedByteArray8) == 8);

}} // namespace tdc::pred
