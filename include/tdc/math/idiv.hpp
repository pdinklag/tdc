#pragma once

#include <cstdint>
#include <utility>

#include <tdc/util/concepts.hpp>

namespace tdc {
namespace math {

/// \brief Computes the rounded-up integer quotient of two numbers.
/// \tparam A the type of the first integer
/// \tparam B the type of the second integer
/// \param a the dividend
/// \param b the divisor
template<typename A, typename B>
requires std::integral<A> && std::integral<B> && Arithmetic<A> && Arithmetic<B> && Arithmetic<decltype(std::declval<A>() + std::declval<B>())>
constexpr auto idiv_ceil(const A a, const B b) {
    using C = decltype(a + b);
    return ((a + b) - C(1ULL)) / b;
}

}} // namespace tdc::math
