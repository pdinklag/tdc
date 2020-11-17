#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>

namespace tdc {
namespace intrisics {

/// \brief Returned by \ref select in case the searched bit does not exist in the given input value.
constexpr size_t SELECT_FAIL = SIZE_MAX;

/// \brief Finds the position of the k-th set bit in an integer (LSBF).
///
/// \tparam T the integer type
/// \param v the input value
/// \param k the searched 1-bit
/// \return the position of the k-th set bit (LSBF and zero-based), or \ref SELECT_FAIL if no such bit exists
template<typename T>
constexpr size_t select(T x, size_t k) {
    size_t pos = 0;
    while(x && k && pos < std::numeric_limits<T>::digits) {
        const size_t z = tzcnt(x)+1;
        pos += z;
        x >>= z;
        --k;
    }
    return k ? SELECT_FAIL : pos - 1;
};

}} // namespace tdc::intrisics
