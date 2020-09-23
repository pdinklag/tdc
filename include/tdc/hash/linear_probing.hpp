#pragma once

#include <cstdint>

namespace tdc {
namespace hash {

/// \brief Linear probing in a hash \ref Map.
/// \tparam m_inc the linear probing step length
template<size_t m_inc = 1>
struct LinearProbing {
    inline size_t operator()(const size_t i) {
        return i + m_inc;
    }
};

}} // namespace tdc::hash
