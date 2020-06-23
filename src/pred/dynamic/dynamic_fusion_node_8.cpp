#include <cassert>

#include <bitset> // FIXME: Debug
#include <iostream> // FIXME: Debug

constexpr size_t NUM_DEBUG_BITS = 24;

#include <tdc/math/bit_mask.hpp>
#include <tdc/pred/fusion_node_internals.hpp>
#include <tdc/pred/dynamic/dynamic_fusion_node_8.hpp>
#include <tdc/pred/util/packed_byte_array_8.hpp>
#include <tdc/util/likely.hpp>

using namespace tdc::pred::dynamic;

DynamicFusionNode8::DynamicFusionNode8() : m_size(0) {
}

size_t DynamicFusionNode8::find(const uint64_t key) const {
    // FIXME: naive, use data structure
    for(size_t i = 0; i < size(); i++) {
        if(m_key[i] == key) return i;
    }
    return SIZE_MAX;
}

uint64_t DynamicFusionNode8::select(const size_t i) const {
    assert(i < size());
    return m_key[i];
}

Result DynamicFusionNode8::predecessor(const uint64_t x) const {
    if(tdc_unlikely(size() == 0)) {
        return { false, 0 };
    } else {
        return internal::predecessor(*this, x, { m_mask, m_branch, m_free });
    }
}

void DynamicFusionNode8::insert(const uint64_t key) {
    const size_t sz = size();
    assert(sz < 8);

    // find rank
    internal::ExtResult xr;
    size_t key_rank;
    if(sz > 0) {
        // find
        xr = internal::predecessor_ext(*this, key, { m_mask, m_branch, m_free });
        key_rank = xr.r.exists ? xr.r.pos + 1 : 0;
    } else {
        // first key
        key_rank = 0;
    }
    
    const size_t i = key_rank;
    assert(i < 8);
    if(i < sz) assert(key != select(i)); // keys must be unique
    //~ std::cout << "insert key = " << std::bitset<NUM_DEBUG_BITS>(key) << std::endl;
    //~ std::cout << "    i = " << i << std::endl;

    // update data structure
    if(sz > 0)
    {
        // get match results
        const size_t j = xr.j;
        const size_t i0 = xr.i0;
        const size_t i1 = xr.i1;
        const size_t matched = xr.i_matched;
        
        //~ std::cout << "    j = " << j;
        
        const uint64_t jmask = 1ULL << (63ULL - j);
        const bool new_significant_position = (m_mask & jmask) == 0;
        
        //~ if(new_significant_position) std::cout << " (new significant position)";
        //~ std::cout << std::endl;
        
        //~ std::cout << "    jmask = " << std::bitset<NUM_DEBUG_BITS>(jmask) << std::endl;
        //~ std::cout << "    i0 = " << i0 << std::endl;
        //~ std::cout << "    i1 = " << i1 << std::endl;

        // find rank h of bit j (in old mask)
        const size_t h = j < 63 ? __builtin_popcountll(m_mask << (j+1)) : 0;
        const uint8_t hset = 1U << h;
        const uint8_t hclear = ~hset;
        const uint8_t hmask_lo = math::bit_mask(h);
        const uint8_t hmask_hi = ~hmask_lo;

        const uint64_t matrix_hset = 0x0101010101010101ULL << h;
        //~ std::cout << "    h = " << h << std::endl;
        
        // update mask (compression function)
        m_mask |= jmask;
        
        // update matrix
        if(new_significant_position) {
            // insert a column of dontcares (0 in branch, 1 in free)
            const uint64_t matrix_hmask_lo = hmask_lo * 0x0101010101010101ULL;
            const uint64_t matrix_hmask_hi = hmask_hi * 0x0101010101010101ULL;

            // make sure compressed keys that aren't used stay at the maximum possible value
            const uint64_t unused = UINT64_MAX << (8 * sz);

            // set h bit accordingly and shift all bits >= h one to the left
            m_branch =               ((m_branch & matrix_hmask_hi) << 1ULL) | (m_branch & matrix_hmask_lo)   | unused;
            m_free   = (matrix_hset | ((m_free   & matrix_hmask_hi) << 1ULL) | (m_free   & matrix_hmask_lo)) & ~unused;
        }

        // update h-bits of compressed keys between ranks i0 and i1
        {
            // get the significant bit in the matched key
            const uint64_t y = select(matched);
            const bool y_j = (y & jmask) != 0;

            // create a mask for rows i0 to i1
            const uint64_t matrix_i0_i1 = math::bit_mask((i1+1) * 8) - math::bit_mask(i0 * 8);

            // mask out bit h
            const uint64_t matrix_h_i0_i1 = matrix_i0_i1 & matrix_hset;

            // set branch bits according to bit in matched key
            m_branch |= (matrix_h_i0_i1 * y_j);

            // clear free bits
            m_free &= ~matrix_h_i0_i1;
        }
        
        // insert new row at rank i
        {
            const uint8_t matched_branch = m_branch >> (matched * 8ULL);
            const uint8_t matched_free = m_free >> (matched * 8ULL);

            // construct new entry:
            // - the h-1 lowest bits are dontcares (branch 0, free 1)
            // - the h-bit equals the j-th bit in the key
            // - the remaining high bits equal that of the matched key
            const uint64_t row_branch = (matched_branch & (~hmask_lo << 1U)) | (((key & jmask) != 0) << h);
            const uint64_t row_free = (matched_free & hclear) | (UINT8_MAX & hmask_lo);

            const uint64_t lo = math::bit_mask(i * 8ULL);
            m_branch = (m_branch & lo) | ((m_branch & ~lo) << 8ULL) | (row_branch << (i * 8ULL));
            m_free   = (m_free & lo)   | ((m_free & ~lo) << 8ULL)   | (row_free << (i * 8ULL));
        }
    } else {
        // we just inserted the first key
        m_mask = 0;                    // there are no branches in the trie
        m_branch = UINT64_MAX << 8ULL; // the compressed key is 0
        m_free = 0;                    // there are no wildcards
    }
    
    // insert
    {
        for(size_t j = sz; j > i; j--) {
            m_key[j] = m_key[j-1];
        }
        m_key[i] = key;
        ++m_size;
    }
    
    // verify
    {
        //~ auto fnode = internal::construct(*this, sz + 1);
        //~ std::cout << "    => mask   ~> " << std::bitset<NUM_DEBUG_BITS>(fnode.mask) << std::endl;
        //~ std::cout << "    => branch ~> 0x" << std::hex << fnode.branch << std::endl;
        //~ std::cout << "    => free   ~> 0x" << fnode.free << std::dec << std::endl;
        //~ assert(m_mask == fnode.mask);
        //~ assert(m_branch == fnode.branch);
        //~ assert(m_free == fnode.free);
    }
}

bool DynamicFusionNode8::remove(const uint64_t key) {
    const size_t sz = size();
    assert(sz > 0);

    // find key
    const size_t i = find(key);
    if(i < sz) {
        // std::cout << "removing key " << key << " with rank " << i << " from slot " << j << std::endl;
        
        // remove
        for(size_t j = i; j < sz - 1; j++) {
            m_key[j] = m_key[j+1];
        }
        --m_size;

        // nb: a columns becomes insignificant when it contains wildcards in all but one row
        
        // update data structure
        // FIXME: for now, we update data structure the naive way,
        // meaning that we put the keys in order and build a static fusion node over it
        // any time the trie gets updated
        if(sz > 0) {
            auto fnode = internal::construct(*this, size());
            m_mask = fnode.mask;
            m_branch = fnode.branch;
            m_free = fnode.free;
        }
        
        return true;
    } else {
        // key not found
        return false;
    }
}

void DynamicFusionNode8::print() const {
    std::cout << "DynamicFusionNode8 (size=" << size() << "):" << std::endl;

    for(size_t i = 0; i < size(); i++) {
        std::cout << "\ti=" << i << " -> keys[i]=" << m_key[i] << std::endl;
    }
}
