#pragma once

#include <cstdint>

namespace tdc {

/// \brief The type to use for indices into an input.
///
/// This can be used to optimize the memory footprint for applications if it is known that inputs never exceed a certain size.
using index_t = uint64_t;

} // namespace tdc
