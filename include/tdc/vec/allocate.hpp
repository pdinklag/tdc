#pragma once

#include <cstring>
#include <memory>
#include <utility>

namespace tdc {
namespace vec {

/// \cond INTERNAL
std::unique_ptr<uint64_t[]> allocate_integers(const size_t num, const size_t width, const bool initialize = true);
/// \endcond

}} // namespace tdc::vec
