#include <cassert>

#include <bitset> // FIXME: Debug
#include <iostream> // FIXME: Debug

constexpr size_t NUM_DEBUG_BITS = 24;

#include <tdc/math/bit_mask.hpp>
#include <tdc/pred/dynamic/btree/dynamic_fusion_node.hpp>
#include <tdc/util/likely.hpp>
#include <tdc/util/rank_u64.hpp>
#include <tdc/util/select_u64.hpp>

using namespace tdc::pred::dynamic;

DynamicFusionNode::DynamicFusionNode() : m_size(0) {
}

size_t DynamicFusionNode::find(const uint64_t key) const {
    // FIXME: naive, use data structure
    for(size_t i = 0; i < size(); i++) {
        if(m_key[i] == key) return i;
    }
    return SIZE_MAX;
}

uint64_t DynamicFusionNode::select(const size_t i) const {
    assert(i < size());
    return m_key[i];
}

Result DynamicFusionNode::predecessor(const uint64_t x) const {
    const size_t sz = size();
    if(tdc_unlikely(sz == 0)) {
        return { false, 0 };
    } else {
        return Internals::predecessor(*this, x, m_mask, m_branch, m_free);
    }
}

void DynamicFusionNode::insert(const uint64_t key) {
    const size_t sz = size();
    assert(sz < 8);

    // find rank
    internal::ExtResult xr;
    size_t key_rank;
    if(sz > 0) {
        // find
        xr = Internals::predecessor_ext(*this, key, m_mask, m_branch, m_free);
        key_rank = xr.r.exists ? xr.r.pos + 1 : 0;
    } else {
        // first key
        key_rank = 0;
    }
    
    const size_t i = key_rank;
    assert(i < 8);

    // keys must be unique
    if(i > 0) assert(key != select(i - 1));
    if(i < sz) assert(key != select(i));

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
        const size_t h = j < 63 ? rank1_u64(m_mask << (j+1)) : 0;
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
            m_branch =                ((m_branch & matrix_hmask_hi) << 1ULL) | (m_branch & matrix_hmask_lo)  |  unused;
            m_free   = (matrix_hset | ((m_free  & matrix_hmask_hi) << 1ULL)  | (m_free   & matrix_hmask_lo)) & ~unused;
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
    //~ {
        //~ auto fnode = internal::construct(*this, sz + 1);
        //~ if(m_branch != fnode.branch || m_free != fnode.free || m_mask != fnode.mask) {
            //~ std::cout << "m_mask   is 0x" << std::hex << m_mask << ", should be 0x" << fnode.mask << std::endl;
            //~ std::cout << "m_branch is 0x" << std::hex << m_branch << ", should be 0x" << fnode.branch << std::endl;
            //~ std::cout << "m_free   is 0x" << std::hex << m_free << ", should be 0x" << fnode.free << std::endl;
        //~ }

        //~ assert(m_mask == fnode.mask);
        //~ assert(m_branch == fnode.branch);
        //~ assert(m_free == fnode.free);
    //~ }
}

bool DynamicFusionNode::remove(const uint64_t key) {
    const size_t sz = size();
    assert(sz > 0);

    // find key
    const auto r = Internals::predecessor(*this, key, m_mask, m_branch, m_free);
    const size_t i = r.pos;
    
    if(tdc_unlikely(!r.exists || select(i) != key)) return false; // key does not exist
    
    //~ std::cout << "removing key " << key << std::endl;
    //~ std::cout << "\ti=" << i << std::endl;
    
    //
    if(sz > 2) {
        // find least significant bit (in compressed key) of key that is not a wildcard
        // to do that, check the number of trailing ones in free
        const uint8_t key_free = m_free >> (i * 8);
        const uint8_t key_branch = m_branch >> (i * 8);
        
        const size_t h = __builtin_ctz(~key_free); // free contains a 1 for every wildcard, so invert free and count the trailing zeroes
        //~ std::cout << "\th=" << h << std::endl;

        // find j, the position (h+1)-th set bit in the mask
        const size_t j = select1_u64(m_mask, h+1);
        //~ std::cout << "\tj=" << j << std::endl;

        // create masks for removal of i-th column
        const uint64_t rm_lo = math::bit_mask(i * 8ULL);
        const uint64_t rm_hi = ~rm_lo << 8ULL;

        // extract h-th column from branch and free (where column i is already removed!)
        bool remove_column;
        {
            const uint64_t branch_rm = (m_branch & rm_lo) | ((m_branch & rm_hi) >> 8ULL);
            const uint64_t free_rm   = (m_free & rm_lo)   | ((m_free & rm_hi) >> 8ULL);
            
            const uint64_t szmask = math::bit_mask((sz - 1) * 8);
            const uint64_t hextract = (0x0101010101010101ULL << h) & szmask;
            
            const uint64_t hbranch = (branch_rm & hextract) & szmask;
            const uint64_t hfree  =  (free_rm & hextract) & szmask;

            // test if j is no longer a significant position
            remove_column =
                hbranch == 0                        // all non-wildcard branch bits are zero
                || ((hbranch | hfree) == hextract); // all non-wildcard branch bits are one

            //~ std::cout << "\thbranch=0x" << std::hex << hbranch << std::endl;
            //~ std::cout << "\thfree=0x" << hfree << std::endl;
            //~ std::cout << "\thextract=0x" << hextract << std::endl;
            //~ std::cout << "\tremove_column=" << std::dec << remove_column << std::endl;
        }
        
        if(remove_column) {
            // remove column h
            const uint8_t hmask_lo = math::bit_mask(h);
            const uint8_t hmask_hi = ~hmask_lo << 1ULL;
            
            const uint64_t matrix_hmask_lo = hmask_lo * 0x0101010101010101ULL;
            const uint64_t matrix_hmask_hi = hmask_hi * 0x0101010101010101ULL;
            
            m_branch = ((m_branch & matrix_hmask_hi) >> 1ULL) | (m_branch & matrix_hmask_lo);
            m_free   = ((m_free   & matrix_hmask_hi) >> 1ULL) | (m_free   & matrix_hmask_lo);

            // clear the j-th bit in mask
            m_mask &= ~(1ULL << j);
        } else {
            // find sibling subtree
            const size_t i0 = Internals::match(key & (UINT64_MAX << (j+1)), m_mask, m_branch, m_free);
            const size_t i1 = Internals::match(key | (UINT64_MAX >> (63ULL - j)), m_mask, m_branch, m_free);
            //~ std::cout << "\ti0=" << std::dec << i0 << ", i1=" << i1 << std::endl;

            // set the h-bits in the subtree elements to be wildcards

            // create a mask for rows i0 to i1
            const uint64_t matrix_i0_i1 = math::bit_mask((i1+1) * 8) - math::bit_mask(i0 * 8);

            // mask out bit h
            const uint64_t matrix_hset = 0x0101010101010101ULL << h;
            const uint64_t matrix_h_i0_i1 = matrix_i0_i1 & matrix_hset;

            // clear branch bits and set free bits
            m_branch &= ~matrix_h_i0_i1;
            m_free   |= matrix_h_i0_i1;
        }

        // remove the i-th row from the matrix
        m_branch = (m_branch & rm_lo) | ((m_branch & rm_hi) >> 8ULL);
        m_free   = (m_free & rm_lo)   | ((m_free & rm_hi) >> 8ULL);
        
        // make sure compressed keys that aren't used are at the maximum possible value
        {
            const uint64_t unused = UINT64_MAX << (8 * (sz-1));
            m_branch |=  unused;
            m_free   &= ~unused;
        }
    } else if(sz == 2) {
        // only one key is left
        m_mask = 0;                    // there are no branches in the trie
        m_branch = UINT64_MAX << 8ULL; // the compressed key is 0
        m_free = 0;                    // there are no wildcards
    } else {
        // removing the last key, no update really needed
    }

    // remove
    //~ std::cout << "\tkeys <- ";
    //~ for(size_t k = 0; k < i; k++) std::cout << std::dec << m_key[k] << " ";
    for(size_t k = i; k < sz - 1; k++) {
        m_key[k] = m_key[k+1];
        //~ std::cout << m_key[k] << " ";
    }
    //~ std::cout << std::endl;
    --m_size;

    // verify
    //~ if(sz > 1) {
        //~ auto fnode = internal::construct(*this, size());
        //~ if(m_branch != fnode.branch || m_free != fnode.free || m_mask != fnode.mask) {
            //~ std::cout << "m_mask   is 0x" << std::hex << m_mask << ", should be 0x" << fnode.mask << std::endl;
            //~ std::cout << "m_branch is 0x" << std::hex << m_branch << ", should be 0x" << fnode.branch << std::endl;
            //~ std::cout << "m_free   is 0x" << std::hex << m_free << ", should be 0x" << fnode.free << std::endl;
        //~ }
        
        //~ assert(m_mask == fnode.mask);
        //~ assert(m_branch == fnode.branch);
        //~ assert(m_free == fnode.free);
    //~ }
    
    return true;
}

void DynamicFusionNode::print() const {
    std::cout << "DynamicFusionNode (size=" << size() << "):" << std::endl;

    for(size_t i = 0; i < size(); i++) {
        std::cout << "\ti=" << i << " -> keys[i]=" << m_key[i] << std::endl;
    }
}
