#pragma once

#include <cstddef>
#include <cstdint>
#include <tdc/math/iminmax.hpp>

#include <iostream>

namespace tdc {
namespace intrisics {

/// \brief Counts the number of trailing zero bits in an integer.
/// \tparam T the integer type
/// \param x the integer
template<typename T>
size_t tzcnt(const T x);

/// \cond INTERNAL
// nb: there seems to a GCC bug concerning compile-time evaluation of __builtin_ctz and friends
// for that reason, tzcnt cannot be constexpr and we have to force runtime computations using volatile variables
// see https://stackoverflow.com/questions/64695111/gcc-wrong-compile-time-evaluation-of-builtin-ctz-in-some-situations-with-o2 for details

template<>
inline size_t tzcnt(const uint8_t x) {
    volatile int r = __builtin_ctz(x); 
    return math::imin((size_t)r, size_t(8)); // there can be at most 8 trailing zeroes
};

template<>
inline size_t tzcnt(const uint16_t x) {
    volatile int r = __builtin_ctz(x);
    return math::imin((size_t)r, size_t(16)); // there can be at most 16 trailing zeroes
};

template<>
inline size_t tzcnt(const uint32_t x) {
    volatile int r = __builtin_ctz(x);
    return r;
};

template<>
inline size_t tzcnt(const uint64_t x) {
    volatile int r = __builtin_ctzll(x);
    return r;
};




/// \endcond

}} // namespace tdc::intrisics

