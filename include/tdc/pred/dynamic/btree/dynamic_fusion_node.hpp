#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <limits>

#include <bitset> // FIXME: Debug
#include <iostream> // FIXME: Debug

#include <tdc/math/bit_mask.hpp>
#include <tdc/intrisics/popcnt.hpp>
#include <tdc/intrisics/select.hpp>
#include <tdc/intrisics/tzcnt.hpp>
#include <tdc/util/concepts.hpp>
#include <tdc/util/likely.hpp>

#include <tdc/pred/fusion_node_internals.hpp>
#include <tdc/pred/result.hpp>

namespace tdc {
namespace pred {
namespace dynamic {

using PosResult = ::tdc::pred::PosResult;

/// \brief A dynamic fusion node for B-trees as per Patrascu & Thorup, 2014.
/// \tparam the key type
/// \tparam the maximum number of keys
template<std::totally_ordered key_t = uint64_t, size_t m_max_keys = 8, bool linear_rank = false>
class DynamicFusionNode {
public:
    /// \brief Reports whether the keys in this data structure are in ascending order.
    static constexpr bool is_ordered() {
        return true;
    }

private:
    static constexpr size_t NUM_DEBUG_BITS = 24; // FIXME: Debug

    static_assert(m_max_keys < 256);
    static constexpr size_t m_key_bits = std::numeric_limits<key_t>::digits;
    static constexpr size_t m_key_msb = m_key_bits - 1;
    static constexpr key_t m_key_max = std::numeric_limits<key_t>::max();

    using Internals = tdc::pred::internal::FusionNodeInternals<key_t, m_max_keys, linear_rank>;
    using mask_t = typename Internals::mask_t;
    using ckey_t = typename Internals::ckey_t;
    using matrix_t = typename Internals::matrix_t;
    
    static constexpr size_t m_ckey_bits = 8ULL * sizeof(ckey_t);
    static constexpr matrix_t m_matrix_max = std::numeric_limits<matrix_t>::max();

    key_t    m_key[m_max_keys];
    mask_t   m_mask;
    matrix_t m_branch, m_free;
    uint8_t  m_size;

public:
    /// \brief Constructs an empty fusion node.
    DynamicFusionNode() : m_size(0) {
    }

    DynamicFusionNode(const DynamicFusionNode&) = default;
    DynamicFusionNode(DynamicFusionNode&&) = default;

    DynamicFusionNode& operator=(const DynamicFusionNode&) = default;
    DynamicFusionNode& operator=(DynamicFusionNode&&) = default;

    /// \brief Selects the key with the given rank.
    /// \param i the rank in question
    key_t select(const size_t i) const {
        assert(i < size());
        return m_key[i];
    }

    /// \brief Convenience operator for \ref select.
    /// \param i the rank in question    
    inline key_t operator[](const size_t i) const {
        return select(i);
    }
    
    /// \brief Finds the rank of the predecessor of the specified key in the node.
    /// \param x the key in question
    PosResult predecessor(const key_t x) const {
        const size_t sz = size();
        if(tdc_unlikely(sz == 0)) {
            return { false, 0 };
        } else {
            return Internals::predecessor(*this, x, m_mask, m_branch, m_free);
        }
    }

    /// \brief Inserts the specified key.
    /// \param key the key to insert
    void insert(const key_t key) {
        const size_t sz = size();
        assert(sz < m_max_keys);

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
        assert(i < m_max_keys);

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
            
            const mask_t jmask = (mask_t)1ULL << (mask_t)(m_key_msb - j);
            const bool new_significant_position = (m_mask & jmask) == 0;
            
            //~ if(new_significant_position) std::cout << " (new significant position)";
            //~ std::cout << std::endl;
            
            //~ std::cout << "    jmask = " << std::bitset<NUM_DEBUG_BITS>(jmask) << std::endl;
            //~ std::cout << "    i0 = " << i0 << std::endl;
            //~ std::cout << "    i1 = " << i1 << std::endl;

            // find rank h of bit j (in old mask)
            const size_t h = j < m_key_msb ? intrisics::popcnt(m_mask << (j+1)) : 0;
            const ckey_t hset = (ckey_t)1U << h;
            const ckey_t hclear = ~hset;
            const ckey_t hmask_lo = math::bit_mask<ckey_t>(h);
            const ckey_t hmask_hi = ~hmask_lo;

            const matrix_t matrix_hset = Internals::REPEAT_MUL << h;
            //~ std::cout << "    h = " << h << std::endl;
            
            // update mask (compression function)
            m_mask |= jmask;
            
            // update matrix
            if(new_significant_position) {
                // insert a column of dontcares (0 in branch, 1 in free)
                const matrix_t matrix_hmask_lo = Internals::repeat(hmask_lo);
                const matrix_t matrix_hmask_hi = Internals::repeat(hmask_hi);

                // make sure compressed keys that aren't used stay at the maximum possible value
                const matrix_t unused = m_matrix_max << (m_ckey_bits * sz);

                // set h bit accordingly and shift all bits >= h one to the left
                m_branch =                ((m_branch & matrix_hmask_hi) << matrix_t(1)) | (m_branch & matrix_hmask_lo)  |  unused;
                m_free   = (matrix_hset | ((m_free  & matrix_hmask_hi) << matrix_t(1))  | (m_free   & matrix_hmask_lo)) & ~unused;
            }

            // update h-bits of compressed keys between ranks i0 and i1
            {
                // get the significant bit in the matched key
                const key_t y = select(matched);
                const bool y_j = (y & jmask) != 0;

                // create a mask for rows i0 to i1
                const matrix_t matrix_i0_i1 = math::bit_mask<matrix_t>((i1+1) * m_ckey_bits) - math::bit_mask<matrix_t>(i0 * m_ckey_bits);

                // mask out bit h
                const matrix_t matrix_h_i0_i1 = matrix_i0_i1 & matrix_hset;

                // set branch bits according to bit in matched key
                m_branch |= (matrix_h_i0_i1 & (-matrix_t(y_j))); // AND with -y_j corresponds to a multiplication with y_j, since y_j is a bool

                // clear free bits
                m_free &= ~matrix_h_i0_i1;
            }
            
            // insert new row at rank i
            {
                const ckey_t matched_branch = m_branch >> (matched * m_ckey_bits);
                const ckey_t matched_free = m_free >> (matched * m_ckey_bits);

                // construct new entry:
                // - the h-1 lowest bits are dontcares (branch 0, free 1)
                // - the h-bit equals the j-th bit in the key
                // - the remaining high bits equal that of the matched key
                const matrix_t row_branch = matrix_t(matched_branch & (~hmask_lo << 1U)) | ((matrix_t)((key & jmask) != 0) << h);
                const matrix_t row_free = matrix_t(matched_free & hclear) | matrix_t(std::numeric_limits<ckey_t>::max() & hmask_lo);

                const matrix_t lo = math::bit_mask<matrix_t>(i * m_ckey_bits);
                m_branch = (m_branch & lo) | ((m_branch & ~lo) << m_ckey_bits) | (row_branch << (i * m_ckey_bits));
                m_free   = (m_free & lo)   | ((m_free & ~lo) << m_ckey_bits)   | (row_free << (i * m_ckey_bits));
            }
        } else {
            // we just inserted the first key
            m_mask = 0;                             // there are no branches in the trie
            m_branch = m_matrix_max << m_ckey_bits; // the compressed key is 0
            m_free = 0;                             // there are no wildcards
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
#ifndef NDEBUG
        /*
        {
            auto fnode = Internals::template construct<key_t>(*this, sz + 1);
            
            auto expected_mask = std::get<0>(fnode);
            auto expected_branch = std::get<1>(fnode);
            auto expected_free = std::get<2>(fnode);
            
            if(m_branch != expected_branch || m_free != expected_free || m_mask != expected_mask) {
                std::cout << "key to be inserted is " << std::dec << key << std::endl;
                std::cout << "m_mask   is 0x" << std::hex << m_mask << ", should be 0x" << expected_mask << std::endl;
                std::cout << "m_branch is 0x" << std::hex << m_branch << ", should be 0x" << expected_branch << std::endl;
                std::cout << "m_free   is 0x" << std::hex << m_free << ", should be 0x" << expected_free << std::endl;
                std::cout << "keys after insert are";
                for(size_t i = 0; i < m_size; i++) std::cout << " " << std::dec << m_key[i];
                std::cout << std::endl;
            }

            assert(m_mask == expected_mask);
            assert(m_branch == expected_branch);
            assert(m_free == expected_free);
        }
        */
#endif
    }

    /// \brief Removes the specified key.
    /// \param key the key to remove
    /// \return whether the item was found and removed
    bool remove(const key_t key) {
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
            // find least significant distinguishing bit that is not a wildcard
            // to do that, check the number of trailing ones in free
            const ckey_t key_free = m_free >> (i * m_ckey_bits);
            const ckey_t key_branch = m_branch >> (i * m_ckey_bits);
            
            const size_t h = intrisics::tzcnt0((ckey_t)~key_free); // free contains a 1 for every wildcard, so invert free and count the trailing zeroes
            //~ std::cout << "\th=" << h << std::endl;

            // find j, the position of the (h+1)-th set bit in the mask
            const size_t j = intrisics::select(m_mask, h+1);
            //~ std::cout << "\tj=" << j << std::endl;

            // create masks for removal of i-th column
            const matrix_t rm_lo = math::bit_mask<matrix_t>(i * m_ckey_bits);
            const matrix_t rm_hi = ~rm_lo << m_ckey_bits;

            // extract h-th column from branch and free (where column i is already removed!)
            bool remove_column;
            {
                const matrix_t branch_rm = (m_branch & rm_lo) | ((m_branch & rm_hi) >> m_ckey_bits);
                const matrix_t free_rm   = (m_free & rm_lo)   | ((m_free & rm_hi) >> m_ckey_bits);
                
                const matrix_t szmask = math::bit_mask<matrix_t>((sz - 1) * m_ckey_bits);
                const matrix_t hextract = (Internals::REPEAT_MUL << h) & szmask;
                
                const matrix_t hbranch = (branch_rm & hextract) & szmask;
                const matrix_t hfree  =  (free_rm & hextract) & szmask;

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
                const ckey_t hmask_lo = math::bit_mask<ckey_t>(h);
                const ckey_t hmask_hi = ~hmask_lo << 1ULL;
                
                const matrix_t matrix_hmask_lo = Internals::repeat(hmask_lo);
                const matrix_t matrix_hmask_hi = Internals::repeat(hmask_hi);
                
                m_branch = ((m_branch & matrix_hmask_hi) >> matrix_t(1)) | (m_branch & matrix_hmask_lo);
                m_free   = ((m_free   & matrix_hmask_hi) >> matrix_t(1)) | (m_free   & matrix_hmask_lo);

                // clear the j-th bit in mask
                m_mask &= ~((mask_t)1ULL << j);
            } else {
                // find sibling subtree
                const size_t i0 = Internals::match(key & ((m_key_max << (key_t)j) << key_t(1)), m_mask, m_branch, m_free); // don't shift by j+1, as j may be m_key_msb-1
                const size_t i1 = Internals::match(key | (m_key_max >> (key_t)(m_key_msb - j)), m_mask, m_branch, m_free);
                //~ std::cout << "\ti0=" << std::dec << i0 << ", i1=" << i1 << std::endl;

                // set the h-bits in the subtree elements to be wildcards

                // create a mask for rows i0 to i1
                const matrix_t matrix_i0_i1 = math::bit_mask<matrix_t>((i1+1) * m_ckey_bits) - math::bit_mask<matrix_t>(i0 * m_ckey_bits);

                // mask out bit h
                const matrix_t matrix_hset = Internals::REPEAT_MUL << h;
                const matrix_t matrix_h_i0_i1 = matrix_i0_i1 & matrix_hset;

                // clear branch bits and set free bits
                m_branch &= ~matrix_h_i0_i1;
                m_free   |= matrix_h_i0_i1;
            }

            // remove the i-th row from the matrix
            m_branch = (m_branch & rm_lo) | ((m_branch & rm_hi) >> m_ckey_bits);
            m_free   = (m_free & rm_lo)   | ((m_free & rm_hi) >> m_ckey_bits);
            
            // make sure compressed keys that aren't used are at the maximum possible value
            {
                const matrix_t unused = m_matrix_max << (matrix_t)(m_ckey_bits * (sz-1));
                m_branch |=  unused;
                m_free   &= ~unused;
            }
        } else if(sz == 2) {
            // only one key is left
            m_mask = 0;                             // there are no branches in the trie
            m_branch = m_matrix_max << m_ckey_bits; // the compressed key is 0
            m_free = 0;                             // there are no wildcards
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
#ifndef NDEBUG
        /*
        if(sz > 1) {
            auto fnode = Internals::template construct<key_t>(*this, size());
            
            auto expected_mask = std::get<0>(fnode);
            auto expected_branch = std::get<1>(fnode);
            auto expected_free = std::get<2>(fnode);
            
            if(m_branch != expected_branch || m_free != expected_free || m_mask != expected_mask) {
                std::cout << "key to be removed is " << std::dec << key << std::endl;
                std::cout << "m_mask   is 0x" << std::hex << m_mask << ", should be 0x" << expected_mask << std::endl;
                std::cout << "m_branch is 0x" << std::hex << m_branch << ", should be 0x" << expected_branch << std::endl;
                std::cout << "m_free   is 0x" << std::hex << m_free << ", should be 0x" << expected_free << std::endl;
                std::cout << "keys after removal are";
                for(size_t i = 0; i < m_size; i++) std::cout << " " << std::dec << m_key[i];
                std::cout << std::endl;
            }

            assert(m_mask == expected_mask);
            assert(m_branch == expected_branch);
            assert(m_free == expected_free);
        }
        */
#endif
        
        return true;
    }

    /// \brief Returns the current size of the fusion node.
    inline size_t size() const {
        return m_size;
    }

#ifndef NDEBUG
    void print() const {
        std::cout << "DynamicFusionNode (size=" << size() << "):" << std::endl;

        // print table
        const size_t dbits = intrisics::popcnt(m_mask);
        
        std::cout << "distinguishing bits: " << std::dec << dbits << std::endl;
        std::cout << "M      = 0x" << std::hex << m_mask << std::endl;
        std::cout << "BRANCH = 0x" << std::hex << m_branch << std::endl;
        std::cout << "FREE   = 0x" << std::hex << m_free << std::endl;
        
        std::cout << std::endl;
        std::cout << "i\t| key\t| ckey" << std::endl;
        std::cout << "---------------------------------" << std::endl;
        for(size_t i = 0; i < size(); i++) {
            std::cout << std::dec << i << "\t| " << m_key[i] << "\t| ";
            
            const ckey_t branch_i = (ckey_t)(m_branch >> (i * m_ckey_bits));
            const ckey_t   free_i = (ckey_t)(m_free   >> (i * m_ckey_bits));
            
            ckey_t extract = 1ULL << (dbits - 1);
            while(extract) {
                if(free_i & extract) {
                    std::cout << '?';
                } else {
                    std::cout << ((branch_i & extract) ? '1' : '0');
                }
                extract >>= 1ULL;
            }
            
            std::cout << std::endl;
            // BRANCH=0x" << std::hex << uint64_t(branch_i) << ", FREE=0x" << uint64_t(free_i) << std::endl;
        }
    }
#endif
} __attribute__((__packed__));

}}} // namespace tdc::pred::dynamic
