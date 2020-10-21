#pragma once

#include <bitset>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <tuple>

#include <tdc/intrisics/parallel_bits.hpp>
#include <tdc/intrisics/parallel_compare.hpp>
#include <tdc/intrisics/lzcnt.hpp>
#include <tdc/pred/result.hpp>
#include <tdc/pred/util/packed_byte_array_8.hpp>
#include <tdc/util/assert.hpp>
#include <tdc/util/int_type_traits.hpp>

/// \cond INTERNAL

namespace tdc {
namespace pred {
namespace internal {

uint64_t pext(const uint64_t x, const uint64_t mask);
uint64_t pcmpgtub(const uint64_t a, const uint64_t b);

struct ExtResult {
    Result r;
    size_t j;
    size_t i0, i1;
    size_t i_matched;
};

template<typename key_t, size_t m_max_keys>
class FusionNodeInternals;

template<typename key_t>
class FusionNodeInternals<key_t, 8> {
public:
    using mask_t = key_t; // key compression mask
    using ckey_t = uint8_t; // compressed key
    using matrix_t = uint64_t; // matrix of compressed keys
    
    static constexpr matrix_t REPEAT_MUL = 0x01'01'01'01'01'01'01'01ULL;
    
    static constexpr size_t m_key_bits = int_type_traits<key_t>::num_bits();
    static constexpr key_t m_key_max = std::numeric_limits<key_t>::max();

private:
    // compress a key using a mask (PEXT)
    static ckey_t compress(const key_t key, const mask_t mask) {
        return intrisics::pext(key, mask);
    }

    // repeat a byte eight times into a 64-bit word
    static matrix_t repeat(const ckey_t x) {
        return matrix_t(x) * REPEAT_MUL;
    }

    // finds the rank of a repeated value in a packed array of eight bytes
    static size_t rank(const matrix_t cx_repeat, const matrix_t array) {
        // compare it against the given array of 8 keys (in parallel)

        const auto cmp = intrisics::pcmpgtub(array, cx_repeat);
        
        // find the position of the first key greater than the compressed key
        // this is as easy as counting the trailing zeroes, of which there are a multiple of 8
        const size_t ctz = __builtin_ctzll(cmp) / 8;
        
        // the rank of our key is at the position before
        return ctz - 1;
    }

public:
    // the match operation from Patrascu & Thorup, 2014
    static size_t match(const key_t x, const mask_t mask, const matrix_t branch, const matrix_t free) {
        // compress the key and repeat it eight times
        const ckey_t cx = compress(x, mask);
        const matrix_t cx_repeat = repeat(cx);
        
        // construct our matching array by replacing all dontcares by the corresponding bits of the compressed key
        const auto match_array = branch | (cx_repeat & free);
        
        // now find the rank of the key in that array
        return rank(cx_repeat, match_array);
    }

    // predecessor search
    template<typename array_t>
    static Result predecessor(const array_t& keys, const key_t x, const mask_t mask, const matrix_t branch, const matrix_t free) {
        // std::cout << "predecessor(" << x << "):" << std::endl;
        // find the predecessor candidate by matching the key against our maintained keys
        const size_t i = match(x, mask, branch, free);
        // std::cout << "\ti=" << i << std::endl;
        // lookup the key at the found position
        const key_t y = keys[i];
        // std::cout << "\ty=" << y << std::endl;
        
        if(x == y) {
            // exact match - the predecessor is the key itself
            return Result { true, i };
        } else {
            // mismatch
            // find the common prefix between the predecessor candidate -- which is the longest between x and any trie entry [Fredman and Willard '93]
            // this can be done by finding the most significant bit of the XOR result (which practically marks all bits that are different)
            const size_t j = intrisics::lzcnt<key_t>(x ^ y);
            const key_t jmask_lo = m_key_max >> j;
            const key_t jmask_hi = ~jmask_lo;
            
            // depending on whether x < y, we will find the smallest or largest key below the candidate, respectively
            // computing both match subjects is faster than branching and deciding
            const size_t xj[] = {
                x & jmask_hi,
                x | jmask_lo
            };
            
            const bool x_lt_y = x < y;
            const size_t ixj = match(xj[!x_lt_y], mask, branch, free);
            
            // branchless version of:
            // if(x < y) {
                // return Result { ixj > 0, ixj - 1 };
            // } else {
                // return Result { true, ixj };
            // }
            return Result { !x_lt_y || ixj > 0, ixj - x_lt_y  };
        }
    }
    
    // predecessor search, extended result
    template<typename array_t>
    static ExtResult predecessor_ext(const array_t& keys, const key_t x, const mask_t mask, const matrix_t branch, const matrix_t free) {
        // std::cout << "predecessor(" << x << "):" << std::endl;
        // find the predecessor candidate by matching the key against our maintained keys
        const size_t i = match(x, mask, branch, free);
        // std::cout << "\ti=" << i << std::endl;
        // lookup the key at the found position
        const key_t y = keys[i];
        // std::cout << "\ty=" << y << std::endl;
        
        if(x == y) {
            // exact match - the predecessor is the key itself
            return ExtResult { Result { true, i }, SIZE_MAX, SIZE_MAX, SIZE_MAX, i };
        } else {
            // mismatch
            // find the common prefix between the predecessor candidate -- which is the longest between x and any trie entry [Fredman and Willard '93]
            // this can be done by finding the most significant bit of the XOR result (which practically marks all bits that are different)
            const size_t j = intrisics::lzcnt<key_t>(x ^ y);
            const key_t jmask_lo = m_key_max >> j;
            const key_t jmask_hi = ~jmask_lo;
            
            const size_t i0 = match(x & jmask_hi, mask, branch, free);
            const size_t i1 = match(x | jmask_lo, mask, branch, free);
            
            ExtResult xr;
            xr.j = j;
            xr.i0 = i0;
            xr.i1 = i1;
            xr.i_matched = (x < y) ? i0 : i1;
            xr.r = Result { x >= y || i0 > 0, x < y ? i0 - 1 : i1 };
            return xr;
        }
    }

    // constructs a fusion node for eight keys
    // currently supports 64-bit keys ONLY
    template<typename array_t>
    static std::tuple<mask_t, matrix_t, matrix_t> construct(const array_t& keys, const size_t num)
    {
        // a very simple trie data structure
        struct trie_node {
            uint16_t child[2]; // TODO: why 16 bits??
            
            // tells whether this is a branching node (two children)
            bool is_branch() const {
                return child[0] && child[1];
            }
        };
        
        assert(num > 0);
        assert(num <= 8);
        tdc::assert_sorted_ascending(keys, num);

        // insert keys into binary trie and compute mask
        trie_node trie[num * 64]; // it can't have more than this many nodes
        std::memset(&trie, 0, num * 64 * sizeof(trie_node));
        
        uint16_t next_node = 1;
        const uint16_t root = 0;
        
        mask_t m_mask = 0;
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
                const key_t key = keys[i];
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
};

}}} // namespace tdc::pred::internal

/// \endcond
