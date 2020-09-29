#pragma once

#include <limits>
#include <cstdint>
#include <cstddef>

namespace tdc {

/// \brief Integer-specific type traits.
/// \tparam the integer type
template<typename T>
struct int_type_traits {
    static_assert(std::numeric_limits<T>::is_integer); // only makes sense for integers
    
    /// \brief Reports the number of bits an integer of type \c T takes up.
    static constexpr size_t num_bits() {
        return sizeof(T) * 8ULL;
    }

    /// \brief Reports the position of the most significant bit in LSBF order, with the least significant bit at position zero.
    ///
    /// This typically resolves to \ref num_bits minus one.
    static constexpr size_t msb_pos() {
        return num_bits() - 1ULL;
    }
};

} // namespace tdc
