#pragma once

#include <cstdint>

namespace tdc {
namespace vec {

/// \cond INTERNAL
inline static constexpr uint64_t div64_ceil(const uint64_t num) {
    return (num + 63ULL) >> 6ULL;
}
/// \endcond

}} // namespace tdc::vec
