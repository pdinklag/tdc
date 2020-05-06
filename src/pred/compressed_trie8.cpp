#include <bitset>
#include <cstring>
#include <tuple>

#include <immintrin.h>
#include <nmmintrin.h>
#include <mmintrin.h>

#include <bitset> // FIXME: DEBUG
#include <iostream> // FIXME: DEBUG

#include <tdc/pred/compressed_trie8.hpp>
#include <tdc/util/assert.hpp>
#include <tdc/util/likely.hpp>

using namespace tdc::pred;

struct ctrie8_t {
    uint64_t mask, branch, free;
};

// an 8x8 bit matrix, stuffed into 64 bits
union bit_matrix_8x8_t {
    uint64_t u64;
    uint8_t  row[8];
};

// sanity check
static_assert(sizeof(bit_matrix_8x8_t) == 8);

// a very simple trie data structure
struct trie_node {
    uint16_t child[2];
    
    // tells whether this is a branching node (two children)
    bool is_branch() const {
        return child[0] && child[1];
    }
};

uint64_t compress(const uint64_t key, const uint64_t mask) {
    #ifdef __BMI2__
    return _pext_u64(key, mask);
    #else
    static_assert(false, "BMI2 not supported (when building in Debug mode, you may have to pass -mbmi2 to the compiler)");
    // TODO: provide fallback implementation?
    #endif
}

uint64_t repeat8(const uint8_t x) {
    static constexpr uint64_t REPEAT_MUL = 0x01'01'01'01'01'01'01'01ULL;
    return uint64_t(x) * REPEAT_MUL;
}

size_t rank(const __m64 cx_repeat, const __m64 array8) {
    // compare it against the given array of 8 keys (in parallel)
    // note that _m_pcmpgtb is a comparison for SIGNED bytes, but we want an unsigned comparison
    // this is reached by XORing every value in our array with 0x80
    // this approach was micro-benchmarked against using an SSE max variant, as well as simple byte-wise comparison
    static constexpr __m64 XOR_MASK = (__m64)0x8080808080808080ULL;
    
    const uint64_t cmp = (uint64_t)_m_pcmpgtb(
        _mm_xor_si64(array8, XOR_MASK),
        _mm_xor_si64(cx_repeat, XOR_MASK));
    
    // find the position of the first key greater than the compressed key
    // this is as easy as counting the trailing zeroes, of which there are a multiple of 8
    const size_t ctz = __builtin_ctzll(cmp) / 8;
    
    // the rank of our key is at the position before
    return ctz - 1;
}

size_t match(const uint64_t x, const ctrie8_t& ctrie8) {
    // compress the key and repeat it eight times
    const uint8_t cx = compress(x, ctrie8.mask);
    const uint64_t cx_repeat = repeat8(cx);
    
    // construct our matching array by replacing all dontcares by the corresponding bits of the compressed key
    const uint64_t match_array = ctrie8.branch | (cx_repeat & ctrie8.free);
    
    // now find the rank of the key in that array
    return rank((__m64)cx_repeat, (__m64)match_array);
}

CompressedTrie8::CompressedTrie8() : m_mask(0) {
}

template<typename array_t>
ctrie8_t construct(const array_t& keys, const size_t num)
{
    assert(num > 0);
    assert(num <= CompressedTrie8::MAX_KEYS);
    tdc::assert_sorted_ascending(keys, num);

    // insert keys into binary trie and compute mask
    trie_node trie[num * 64]; // it can't have more than this many nodes
    std::memset(&trie, 0, num * 64 * sizeof(trie_node));
    
    uint16_t next_node = 1;
    const uint16_t root = 0;
    
    size_t m_mask = 0;
    bit_matrix_8x8_t m_branch, m_free;
    
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
            m_branch.row[i] = uint8_t(branch.to_ulong());
            m_free.row[i] = uint8_t(free.to_ulong());
            
            // std::cout << ", branch=" << std::bitset<8>(m_branch.row[i]) << ", free=" << std::bitset<8>(m_free.row[i]) << std::endl;
        }
        
        // in case there are less than 8 entries, repeat an unreachable maximum at the end
        for(; i < CompressedTrie8::MAX_KEYS; i++) {
            m_branch.row[i] = UINT8_MAX;
            m_free.row[i] = 0;
        }
    }
    
    return { m_mask, m_branch.u64, m_free.u64 };
}

CompressedTrie8::CompressedTrie8(const uint64_t* keys, const size_t num) {
    auto ctrie8 = construct(keys, num);
    m_mask = ctrie8.mask;
    m_branch = ctrie8.branch;
    m_free = ctrie8.free;
}

CompressedTrie8::CompressedTrie8(const SkipAccessor<uint64_t>& keys, const size_t num) {
    auto ctrie8 = construct(keys, num);
    m_mask = ctrie8.mask;
    m_branch = ctrie8.branch;
    m_free = ctrie8.free;
}

template<typename array_t>
Result predecessor_internal(const array_t& keys, const uint64_t x, const ctrie8_t& ctrie8) {
    // std::cout << "predecessor(" << x << "):" << std::endl;
    // find the predecessor candidate by matching the key against our maintained keys
    const size_t i = match(x, ctrie8);
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
        const size_t ixj = match(xj[!x_lt_y], ctrie8);
        
        // branchless version of:
        // if(x < y) {
            // return Result { ixj > 0, ixj - 1 };
        // } else {
            // return Result { true, ixj };
        // }
        return Result { !x_lt_y || ixj > 0, ixj - x_lt_y  };
    }
}

Result CompressedTrie8::predecessor(const uint64_t* keys, const uint64_t x) const {
    return predecessor_internal(keys, x, { m_mask, m_branch, m_free });
}

Result CompressedTrie8::predecessor(const SkipAccessor<uint64_t>& keys, const uint64_t x) const {
    return predecessor_internal(keys, x, { m_mask, m_branch, m_free });
}
