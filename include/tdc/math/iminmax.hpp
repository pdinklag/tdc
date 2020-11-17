#pragma once

namespace tdc {
namespace math {

/// \brief Returns the lesser of two integers.
/// \tparam T integer type
/// \param a the first integer
/// \param b the second integer
template<typename T>
constexpr T imin(T a, const T b) {
    return a < b ? a : b; // TODO: branchless version?
}

/// \brief Returns the greater of two integers.
/// \tparam T integer type
/// \param a the first integer
/// \param b the second integer
template<typename T>
constexpr T imax(T a, const T b) {
    return a < b ? b : a; // TODO: branchless version?
}

}} // namespace tdc::math
