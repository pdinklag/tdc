#pragma once

#include <bitset>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <tuple>

#include <tdc/intrisics/lzcnt.hpp>
#include <tdc/intrisics/pcmp.hpp>
#include <tdc/intrisics/pext.hpp>
#include <tdc/intrisics/popcnt.hpp>
#include <tdc/intrisics/tzcnt.hpp>
#include <tdc/pred/result.hpp>
#include <tdc/uint/uint256.hpp>
#include <tdc/util/assert.hpp>
#include <tdc/util/int_type_traits.hpp>

/// \cond INTERNAL

namespace tdc {
namespace pred {
namespace internal {

// constructs a static fusion node for the given keys
template<typename key_t, typename ckey_t, typename mask_t, typename matrix_t, typename array_t>
std::tuple<mask_t, matrix_t, matrix_t> construct_static(const array_t& keys, const size_t num)
{
    assert(int_type_traits<matrix_t>::num_bits() == int_type_traits<ckey_t>::num_bits() * int_type_traits<ckey_t>::num_bits());
    
    const size_t key_bits = int_type_traits<key_t>::num_bits();
    
    // a very simple trie data structure
    using node_t = uint16_t; // we assume we deal with tries with at most 65536 nodes - may be altered at will
    struct trie_node {
        node_t child[2];
        
        // tells whether this is a branching node (two children)
        bool is_branch() const {
            return child[0] && child[1];
        }
    };
    
    assert(num > 0);
    assert(num * num <= int_type_traits<matrix_t>::num_bits());
    tdc::assert_sorted_ascending(keys, num);

    // insert keys into binary trie and compute mask
    assert(num * key_bits * sizeof(trie_node) <= std::numeric_limits<node_t>::max()); // trie too large
    trie_node trie[num * key_bits]; // it can't have more than this many nodes
    std::memset(&trie, 0, num * key_bits * sizeof(trie_node));
    
    node_t next_node = 1;
    const node_t root = 0;
    
    mask_t m_mask = 0;
    const mask_t extract_init = mask_t(1) << int_type_traits<key_t>::msb_pos(); // bit extraction mask, which we will be shifting to the right bit by bit
    
    const size_t max_keys = sizeof(matrix_t) / sizeof(ckey_t);
    ckey_t m_branch[max_keys];
    ckey_t m_free[max_keys];

    for(size_t i = 0; i < num; i++) {
        const key_t key = keys[i];
        mask_t extract = extract_init; 
        node_t v = root; // starting at the root node
        
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
    const size_t num_distinguishing = intrisics::popcnt(m_mask);
    assert(num_distinguishing <= num);
    
    // do a second walk over the trie and build compressed keys with dontcares
    {
        size_t i = 0;
        for(; i < num; i++) {
            const key_t key = keys[i];
            mask_t extract = extract_init;
            node_t v = root; // starting at the root node
            
            std::bitset<64> free, branch; // nb: use of bitset<64> limits ckey_t to uint64_t
            size_t j = num_distinguishing - 1;
            
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
            m_branch[i] = ckey_t(branch.to_ulong());
            m_free[i] = ckey_t(free.to_ulong());
        }
        
        // in case there are less than max_keys entries, repeat an unreachable maximum at the end
        for(; i < max_keys; i++) {
            m_branch[i] = std::numeric_limits<ckey_t>::max();
            m_free[i] = ckey_t(0);
        }
    }
    
    return { m_mask, *((matrix_t*)&m_branch), *((matrix_t*)&m_free) };
}

struct ExtResult {
    PosResult r;
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

    // repeat a byte eight times into a 64-bit word
    static matrix_t repeat(const ckey_t x) {
        return matrix_t(x) * REPEAT_MUL;
    }

private:
    // compress a key using a mask (PEXT)
    static ckey_t compress(const key_t key, const mask_t mask) {
        return (ckey_t)intrisics::pext(key, mask);
    }

    // finds the rank of a repeated value in a packed array of eight bytes
    static size_t rank(const matrix_t cx_repeat, const matrix_t array) {
        // compare it against the given array of 8 keys (in parallel)

        const auto cmp = intrisics::pcmpgtu<matrix_t, ckey_t>(array, cx_repeat);
        
        // find the position of the first key greater than the compressed key
        // this is as easy as counting the trailing zeroes, of which there are a multiple of 8
        const size_t ctz = intrisics::tzcnt(cmp) / 8;
        
        // the rank of our key is at the position before
        return ctz - 1;
    }

    // the match operation from Patrascu & Thorup, 2014
    static size_t match_compressed(const ckey_t cx, const matrix_t branch, const matrix_t free) {
        // repeat the compressed key
        const matrix_t cx_repeat = repeat(cx);
        
        // construct our matching array by replacing all dontcares by the corresponding bits of the compressed key
        const auto match_array = branch | (cx_repeat & free);
        
        // now find the rank of the key in that array
        return rank(cx_repeat, match_array);
    }

public:
    // the match operation from Patrascu & Thorup, 2014
    static size_t match(const key_t x, const mask_t mask, const matrix_t branch, const matrix_t free) {
        const ckey_t cx = compress(x, mask);
        return match_compressed(cx, branch, free);
    }

    // predecessor search
    template<typename array_t>
    static PosResult predecessor(const array_t& keys, const key_t x, const mask_t mask, const matrix_t branch, const matrix_t free) {
        // std::cout << "predecessor(" << x << "):" << std::endl;
        // find the predecessor candidate by matching the key against our maintained keys
        const ckey_t cx = compress(x, mask);
        const size_t i = match_compressed(cx, branch, free);
        // std::cout << "\ti=" << i << std::endl;
        // lookup the key at the found position
        const key_t y = keys[i];
        // std::cout << "\ty=" << y << std::endl;
        
        if(x == y) {
            // exact match - the predecessor is the key itself
            return PosResult { true, i };
        } else {
            // mismatch
            // find the common prefix between the predecessor candidate -- which is the longest between x and any trie entry [Fredman and Willard '93]
            // this can be done by finding the most significant bit of the XOR result (which practically marks all bits that are different)
            const size_t j = intrisics::lzcnt<key_t>(x ^ y);
            const key_t jmask_lo = m_key_max >> j;
            const key_t jmask_hi = ~jmask_lo;
            
            // depending on whether x < y, we will find the smallest or largest key below the candidate, respectively
            // computing both match subjects is faster than branching and deciding
            const key_t xj[] = {
                x & jmask_hi,
                x | jmask_lo
            };
            
            const bool x_lt_y = x < y;
            const ckey_t cx_masked = compress(xj[!x_lt_y], mask);
            const size_t ixj = match_compressed(cx_masked, branch, free);
            
            // branchless version of:
            // if(x < y) {
                // return Result { ixj > 0, ixj - 1 };
            // } else {
                // return Result { true, ixj };
            // }
            return PosResult { !x_lt_y || ixj > 0, ixj - x_lt_y  };
        }
    }
    
    // predecessor search, extended result
    template<typename array_t>
    static ExtResult predecessor_ext(const array_t& keys, const key_t x, const mask_t mask, const matrix_t branch, const matrix_t free) {
        // std::cout << "predecessor(" << x << "):" << std::endl;
        // find the predecessor candidate by matching the key against our maintained keys
        const ckey_t cx = compress(x, mask);
        const size_t i = match_compressed(cx, branch, free);
        // std::cout << "\ti=" << i << std::endl;
        // lookup the key at the found position
        const key_t y = keys[i];
        // std::cout << "\ty=" << y << std::endl;
        
        if(x == y) {
            // exact match - the predecessor is the key itself
            return ExtResult { PosResult { true, i }, SIZE_MAX, SIZE_MAX, SIZE_MAX, i };
        } else {
            // mismatch
            // find the common prefix between the predecessor candidate -- which is the longest between x and any trie entry [Fredman and Willard '93]
            // this can be done by finding the most significant bit of the XOR result (which practically marks all bits that are different)
            const size_t j = intrisics::lzcnt<key_t>(x ^ y);
            const key_t jmask_lo = m_key_max >> j;
            const ckey_t cjmask_lo = compress(jmask_lo, mask); // compress the mask and compute the other from it - saves one PEXT instruction
            const ckey_t cjmask_hi = ~cjmask_lo;

            const size_t i0 = match_compressed(cx & cjmask_hi, branch, free);
            const size_t i1 = match_compressed(cx | cjmask_lo, branch, free);
            
            ExtResult xr;
            xr.j = j;
            xr.i0 = i0;
            xr.i1 = i1;
            xr.i_matched = (x < y) ? i0 : i1;
            xr.r = PosResult { x >= y || i0 > 0, x < y ? i0 - 1 : i1 };
            return xr;
        }
    }

    // constructs a static fusion node
    template<typename array_t>
    static std::tuple<mask_t, matrix_t, matrix_t> construct(const array_t& keys, const size_t num)
    {
        return construct_static<key_t, ckey_t, mask_t, matrix_t>(keys, num);
    }
};

template<typename key_t>
class FusionNodeInternals<key_t, 16> {
public:
    using mask_t = key_t; // key compression mask
    using ckey_t = uint16_t; // compressed key
    using matrix_t = uint256_t; // matrix of compressed keys
    
    static constexpr uint64_t REPEAT_MUL64 = 0x0001'0001'0001'0001ULL;
    static constexpr matrix_t REPEAT_MUL = uint256_t(REPEAT_MUL64, REPEAT_MUL64, REPEAT_MUL64, REPEAT_MUL64);
    
    static constexpr size_t m_key_bits = int_type_traits<key_t>::num_bits();
    static constexpr key_t m_key_max = std::numeric_limits<key_t>::max();

    // repeat a compressed key into a matrix
    static matrix_t repeat(const ckey_t x) {
        const uint64_t x64 = uint64_t(x) * REPEAT_MUL64;
        return uint256_t(x64, x64, x64, x64);
    }

private:
    // compress a key using a mask (PEXT)
    static ckey_t compress(const key_t key, const mask_t mask) {
        return (ckey_t)intrisics::pext(key, mask);
    }

    // finds the rank of a repeated value in a packed array of eight bytes
    static size_t rank(const matrix_t cx_repeat, const matrix_t array) {
        // compare it against the given array of 8 keys (in parallel)

        const auto cmp = intrisics::pcmpgtu<matrix_t, ckey_t>(array, cx_repeat);
        
        // find the position of the first key greater than the compressed key
        // this is as easy as counting the trailing zeroes, of which there are a multiple of 16
        const size_t ctz = intrisics::tzcnt(cmp) / 16;
        
        // the rank of our key is at the position before
        return ctz - 1;
    }

    // the match operation from Patrascu & Thorup, 2014
    static size_t match_compressed(const ckey_t cx, const matrix_t branch, const matrix_t free) {
        // repeat the compressed key
        const matrix_t cx_repeat = repeat(cx);
        
        // construct our matching array by replacing all dontcares by the corresponding bits of the compressed key
        const auto match_array = branch | (cx_repeat & free);
        
        // now find the rank of the key in that array
        return rank(cx_repeat, match_array);
    }

public:
    // the match operation from Patrascu & Thorup, 2014
    static size_t match(const key_t x, const mask_t mask, const matrix_t branch, const matrix_t free) {
        const ckey_t cx = compress(x, mask);
        return match_compressed(cx, branch, free);
    }

    // predecessor search
    template<typename array_t>
    static PosResult predecessor(const array_t& keys, const key_t x, const mask_t mask, const matrix_t branch, const matrix_t free) {
        // std::cout << "predecessor(" << x << "):" << std::endl;
        // find the predecessor candidate by matching the key against our maintained keys
        const ckey_t cx = compress(x, mask);
        const size_t i = match_compressed(cx, branch, free);
        // std::cout << "\ti=" << i << std::endl;
        // lookup the key at the found position
        const key_t y = keys[i];
        // std::cout << "\ty=" << y << std::endl;
        
        if(x == y) {
            // exact match - the predecessor is the key itself
            return PosResult { true, i };
        } else {
            // mismatch
            // find the common prefix between the predecessor candidate -- which is the longest between x and any trie entry [Fredman and Willard '93]
            // this can be done by finding the most significant bit of the XOR result (which practically marks all bits that are different)
            const size_t j = intrisics::lzcnt<key_t>(x ^ y);
            const key_t jmask_lo = m_key_max >> j;
            const key_t jmask_hi = ~jmask_lo;
            
            // depending on whether x < y, we will find the smallest or largest key below the candidate, respectively
            // computing both match subjects is faster than branching and deciding
            const key_t xj[] = {
                x & jmask_hi,
                x | jmask_lo
            };
            
            const bool x_lt_y = x < y;
            const ckey_t cx_masked = compress(xj[!x_lt_y], mask);
            const size_t ixj = match_compressed(cx_masked, branch, free);
            
            // branchless version of:
            // if(x < y) {
                // return Result { ixj > 0, ixj - 1 };
            // } else {
                // return Result { true, ixj };
            // }
            return PosResult { !x_lt_y || ixj > 0, ixj - x_lt_y  };
        }
    }
    
    // predecessor search, extended result
    template<typename array_t>
    static ExtResult predecessor_ext(const array_t& keys, const key_t x, const mask_t mask, const matrix_t branch, const matrix_t free) {
        // std::cout << "predecessor(" << x << "):" << std::endl;
        // find the predecessor candidate by matching the key against our maintained keys
        const ckey_t cx = compress(x, mask);
        const size_t i = match_compressed(cx, branch, free);
        // std::cout << "\ti=" << i << std::endl;
        // lookup the key at the found position
        const key_t y = keys[i];
        // std::cout << "\ty=" << y << std::endl;
        
        if(x == y) {
            // exact match - the predecessor is the key itself
            return ExtResult { PosResult { true, i }, SIZE_MAX, SIZE_MAX, SIZE_MAX, i };
        } else {
            // mismatch
            // find the common prefix between the predecessor candidate -- which is the longest between x and any trie entry [Fredman and Willard '93]
            // this can be done by finding the most significant bit of the XOR result (which practically marks all bits that are different)
            const size_t j = intrisics::lzcnt<key_t>(x ^ y);
            const key_t jmask_lo = m_key_max >> j;
            const ckey_t cjmask_lo = compress(jmask_lo, mask); // compress the mask and compute the other from it - saves one PEXT instruction
            const ckey_t cjmask_hi = ~cjmask_lo;

            const size_t i0 = match_compressed(cx & cjmask_hi, branch, free);
            const size_t i1 = match_compressed(cx | cjmask_lo, branch, free);
            
            ExtResult xr;
            xr.j = j;
            xr.i0 = i0;
            xr.i1 = i1;
            xr.i_matched = (x < y) ? i0 : i1;
            xr.r = PosResult { x >= y || i0 > 0, x < y ? i0 - 1 : i1 };
            return xr;
        }
    }
    
    // constructs a static fusion node
    template<typename array_t>
    static std::tuple<mask_t, matrix_t, matrix_t> construct(const array_t& keys, const size_t num)
    {
        return construct_static<key_t, ckey_t, mask_t, matrix_t>(keys, num);
    }
};

}}} // namespace tdc::pred::internal

/// \endcond
