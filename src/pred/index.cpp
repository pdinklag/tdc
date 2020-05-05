#include <tdc/pred/index.hpp>
#include <tdc/math/ilog2.hpp>
#include <tdc/util/assert.hpp>
#include <tdc/util/likely.hpp>

#include <iostream> // FIXME: Debug

using namespace tdc::pred;

Index::Index(const uint64_t* keys, const size_t num, const size_t lo_bits) : m_lo_bits(lo_bits) {
    assert_sorted_ascending(keys, num);

    // build an index for high bits
    m_min = keys[0];
    m_max = keys[num-1];
    m_key_min = hi(m_min);
    m_key_max = hi(m_max);
    
    // std::cout << "m_min=" << m_min << ", m_key_min=" << m_key_min;
    // std::cout << ", m_max=" << m_max << ", m_key_max=" << m_key_max;
    // std::cout << " -> allocating " << (m_key_max - m_key_min + 2)
        // << " samples with bit width " << math::ilog2_ceil(num-1) << std::endl;

    m_hi_idx = vec::IntVector(m_key_max - m_key_min + 2, math::ilog2_ceil(num-1), false);
    m_hi_idx[0] = 0; // left border of the first interval is the first entry
    assert(m_hi_idx[0] == 0);
    
    // std::cout << "keys[0] = " << keys[0] << ", hi=" << hi(keys[0]) << std::endl;
    // std::cout << "-> sample 0" << std::endl;
    
    uint64_t prev_key = m_key_min;
    for(size_t i = 1; i < num; i++) {
        const uint64_t cur_key = hi(keys[i]);
        // std::cout << "keys[" << i << "] = " << keys[i] << ", hi=" << cur_key << std::endl;
        if(cur_key > prev_key) {
            for(uint64_t key = prev_key + 1; key <= cur_key; key++) {
                m_hi_idx[key - m_key_min] = i - 1;
                // std::cout << "-> sample " << i << std::endl;
                assert(m_hi_idx[0] == 0);
            }
        }
        prev_key = cur_key;
    }

    assert(prev_key == m_key_max);
    m_hi_idx[m_key_max - m_key_min + 1] = num - 1;
    
    // std::cout << "sample: ";
    // for(size_t h = m_key_min; h <= m_key_max; h++) {
        // std::cout << m_hi_idx[h - m_key_min] << ' ';
    // }
    // std::cout << std::endl;

    // build the predecessor data structure for low bits
    m_lo_pred = BinarySearchHybrid(keys, num);
}

Result Index::predecessor(const uint64_t* keys, const size_t num, const uint64_t x) const {
    if(tdc_unlikely(x < m_min))  return Result { false, 0 };
    if(tdc_unlikely(x >= m_max)) return Result { true, num - 1 };

    const uint64_t key = hi(x) - m_key_min;
    assert(key + 1 < m_hi_idx.size());
    const size_t q = m_hi_idx[key+1];

    if(x == keys[q]) {
        return Result { true, q };
    } else {
        const size_t p = m_hi_idx[key];
        return m_lo_pred.predecessor_seeded(keys, p, q, x);
    }
}
