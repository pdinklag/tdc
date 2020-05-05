#pragma once

#include <cstdint>
#include <cstddef>

#include <tdc/vec/int_vector.hpp>

#include "binary_search_hybrid.hpp"
#include "result.hpp"

namespace tdc {
namespace pred {

class Index {
private:
    inline uint64_t hi(uint64_t x) const {
        return x >> m_lo_bits;
    }
    
    size_t m_lo_bits;
    uint64_t m_min, m_max;
    uint64_t m_key_min, m_key_max;

    vec::IntVector m_hi_idx;
    BinarySearchHybrid m_lo_pred;
    
public:
    inline Index() : m_lo_bits(0), m_min(0), m_max(UINT64_MAX), m_key_min(0), m_key_max(UINT64_MAX) {
    }

    Index(const uint64_t* keys, const size_t num, const size_t lo_bits);

    Index(const Index& other) = default;
    Index(Index&& other) = default;
    Index& operator=(const Index& other) = default;
    Index& operator=(Index&& other) = default;

    Result predecessor(const uint64_t* keys, const size_t num, const uint64_t x) const;
};

}} // namespace tdc::pred

