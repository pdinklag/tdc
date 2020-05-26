#pragma once

#include <bitset>
#include <cstddef>
#include <cstdint>
#include <cstring>

#include <mmintrin.h>

#include <tdc/pred/result.hpp>
#include <tdc/pred/util/packed_byte_array_8.hpp>
#include <tdc/util/assert.hpp>

/// \cond INTERNAL

namespace tdc {
namespace pred {
namespace internal {

// a fusion tree node for eight items
struct fnode8_t {
    uint64_t mask, branch, free;
};

// compress a key using a mask (PEXT)
uint64_t compress(const uint64_t key, const uint64_t mask);

// repeat a byte eight times into a 64-bit word
uint64_t repeat8(const uint8_t x);

// finds the rank of a repeated value in a packed array of eight bytes
size_t rank(const __m64 cx_repeat, const __m64 array8);

// the match operation from Patrascu & Thorup, 2014
size_t match(const uint64_t x, const fnode8_t& ctrie8);

// a very simple trie data structure
struct trie_node {
    uint16_t child[2];
    
    // tells whether this is a branching node (two children)
    bool is_branch() const {
        return child[0] && child[1];
    }
};

// constructs a fusion node for eight keys
template<typename array_t>
fnode8_t construct(const array_t& keys, const size_t num)
{
    assert(num > 0);
    assert(num <= 8);
    tdc::assert_sorted_ascending(keys, num);

    // insert keys into binary trie and compute mask
    trie_node trie[num * 64]; // it can't have more than this many nodes
    std::memset(&trie, 0, num * 64 * sizeof(trie_node));
    
    uint16_t next_node = 1;
    const uint16_t root = 0;
    
    size_t m_mask = 0;
    PackedByteArray8 m_branch, m_free;
    
    for(size_t i = 0; i < num; i++) {
        const uint64_t key = keys[i];
        uint64_t extract = 0x8000000000000000ULL; // bit extraction mask, which we will be shifting to the right bit by bit
        uint16_t v = root; // starting at the root node
        
        while(extract) {
            const bool b = key & extract;
            if(!trie[v].child[b]) {
                // insert new node
                trie[v].child[b] = next_node++;
                
                if(trie[v].is_branch()) {
                    // v is on a non-unary path, so we must include this level in the mask
                    m_mask |= extract;
                }
            }
            
            v = trie[v].child[b];
            extract >>= 1ULL;
        }
    }
    // std::cout << "m_mask = " << std::bitset<12>(m_mask) << std::endl;
    
    // sanity check: mask must not have more than num set bits, because there cannot be more inner nodes in the trie
    const size_t num_relevant = __builtin_popcountll(m_mask);
    assert(num_relevant <= num);
    
    // do a second walk over the trie and build compressed keys with dontcares
    {
        size_t i = 0;
        for(; i < num; i++) {
            const uint64_t key = keys[i];
            uint64_t extract = 0x8000000000000000ULL; // bit extraction mask, which we will be shifting to the right bit by bit
            uint16_t v = root; // starting at the root node
            
            std::bitset<8> free, branch;
            size_t j = num_relevant - 1;
            
            // std::cout << i << "\t" << key << "\t= " << std::bitset<12>(key) << " -> ";
            while(extract) {
                const bool b = key & extract;
                const bool m = m_mask & extract;
                
                if(m) { // only look at relevant bits
                    if(trie[v].is_branch()) {
                        // we are on a branching node, add the bit to the compressed key
                        // std::cout << (b ? '1' : '0');
                        branch[j] = b;
                        free[j] = 0;
                    } else {
                        // the node is on a unary path - add a dontcare to the compressed key
                        // std::cout << '?';
                        branch[j] = 0;
                        free[j] = 1;
                    }
                    --j;
                }
                
                v = trie[v].child[b];
                extract >>= 1ULL;
            }
            
            assert(j == SIZE_MAX); // must have counted down to -1
            m_branch.u8[i] = uint8_t(branch.to_ulong());
            m_free.u8[i] = uint8_t(free.to_ulong());
            
            // std::cout << ", branch=" << std::bitset<8>(m_branch.row[i]) << ", free=" << std::bitset<8>(m_free.row[i]) << std::endl;
        }
        
        // in case there are less than 8 entries, repeat an unreachable maximum at the end
        for(; i < 8; i++) {
            m_branch.u8[i] = UINT8_MAX;
            m_free.u8[i] = 0;
        }
    }
    
    return { m_mask, m_branch.u64, m_free.u64 };
}

template<typename array_t>
Result predecessor(const array_t& keys, const uint64_t x, const internal::fnode8_t& fnode8) {
    // std::cout << "predecessor(" << x << "):" << std::endl;
    // find the predecessor candidate by matching the key against our maintained keys
    const size_t i = internal::match(x, fnode8);
    // std::cout << "\ti=" << i << std::endl;
    // lookup the key at the found position
    const uint64_t y = keys[i];
    // std::cout << "\ty=" << y << std::endl;
    
    if(x == y) {
        // exact match - the predecessor is the key itself
        return Result { true, i };
    } else {
        // mismatch
        // find the common prefix between the predecessor candidate -- which is the longest between x and any trie entry [Fredman and Willard '93]
        // this can be done by finding the most significant bit of the XOR result (which practically marks all bits that are different)
        const size_t j = __builtin_clzll(x ^ y);
        
        // depending on whether x < y, we will find the smallest or largest key below the candidate, respectively
        // computing both match subjects is faster than branching and deciding
        const size_t xj[] = {
            x & (UINT64_MAX << (64ULL - j)),
            x | (UINT64_MAX >> j)
        };
        
        const bool x_lt_y = x < y;
        const size_t ixj = internal::match(xj[!x_lt_y], fnode8);
        
        // branchless version of:
        // if(x < y) {
            // return Result { ixj > 0, ixj - 1 };
        // } else {
            // return Result { true, ixj };
        // }
        return Result { !x_lt_y || ixj > 0, ixj - x_lt_y  };
    }
}

}}} // namespace tdc::pred::internal

/// \endcond
