#pragma once

#include <cstdint>

namespace tdc {
namespace math {

/// \brief Computes the rounded-up integer quotient of two numbers.
/// \tparam A the type of the first integer
/// \tparam B the type of the second integer
/// \param a the dividend
/// \param b the divisor
template<typename A, typename B>
constexpr auto idiv_ceil(const A a, const B b) {
    using C = decltype(a + b);
    return ((a + b) - C(1ULL)) / b;
}

}} // namespace tdc::math
