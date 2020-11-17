#pragma once

#include <cstdint>

namespace tdc {
namespace intrisics {

/// \brief Performs a unsigned greater-than comparison of the given packed words.
///
/// The result is a vector of words containing the result of each comparison - the max value if the component from \c a was greater than that from \c b and \c 0x00 otherwise.
///
/// \param T the vector word type
/// \param W the type of words contained in the vector type
/// \param a the first word
/// \param b the second word
template<typename T, typename W>
T pcmpgtu(const T& a, const T& b);
    
}} // namespace tdc::intrisics
