#pragma once

#include <cstdint>

namespace tdc {

template<typename T>
struct uint_half;

template<> struct uint_half<uint16_t> { using type = uint8_t; };
template<> struct uint_half<uint32_t> { using type = uint16_t; };
template<> struct uint_half<uint64_t> { using type = uint32_t; };

template<typename T>
using uint_half_t = uint_half<T>::type;

}
