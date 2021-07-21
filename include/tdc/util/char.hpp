#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>

#ifndef CHAR_T
#pragma message("The CHAR_T macro isn't defined and will therefore default to uint64_t.")
/// \brief Determined by the build system.
#define CHAR_T unsigned char
#endif

namespace tdc {

/// \brief The type to use for indices into an input.
///
/// This can be used to optimize the memory footprint for applications if it is known that inputs never exceed a certain size.
using char_t = CHAR_T;

constexpr char_t CHAR_MAX = std::numeric_limits<char_t>::max();
constexpr size_t CHAR_BITS = std::numeric_limits<char_t>::digits;

} // namespace tdc
