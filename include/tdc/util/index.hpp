#pragma once

#include <cstdint>

#ifndef INDEX_T
#pragma message("The INDEX_T macro isn't defined and will therefore default to uint64_t.")
/// \brief Determined by the build system.
#define INDEX_T uint64_t
#endif

namespace tdc {

/// \brief The type to use for indices into an input.
///
/// This can be used to optimize the memory footprint for applications if it is known that inputs never exceed a certain size.
using index_t = INDEX_T;

} // namespace tdc
