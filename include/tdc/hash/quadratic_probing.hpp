#pragma once

#include <cstdint>

namespace tdc {
namespace hash {

/// \brief Quadratic probing in a hash \ref Map.
///
/// When a collision is found at position \f$ i \f$, the probed position is computed as \f$ ai^2+bi+c \f$
///
/// \tparam m_a probing parameter \f$ a \f$
/// \tparam m_a probing parameter \f$ b \f$
/// \tparam m_a probing parameter \f$ c \f$
template<size_t m_a = 1, size_t m_b = 1, size_t m_c = 1>
struct QuadraticProbing {
    inline size_t operator()(const size_t i) {
        return m_a * i * i + m_b * i + m_c;
    }
};

}} // namespace tdc::hash
