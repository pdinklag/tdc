#include <iostream> // FIXME: DEBUG

#include <tdc/pred/fusion_node_internals.hpp>
#include <tdc/pred/fusion_node_8.hpp>

using namespace tdc::pred;

FusionNode8::FusionNode8() : m_mask(0) {
}

FusionNode8::FusionNode8(const uint64_t* keys, const size_t num) {
    auto fnode8 = internal::construct(keys, num);
    m_mask = fnode8.mask;
    m_branch = fnode8.branch;
    m_free = fnode8.free;
}

FusionNode8::FusionNode8(const SkipAccessor<uint64_t>& keys, const size_t num) {
    auto fnode8 = internal::construct(keys, num);
    m_mask = fnode8.mask;
    m_branch = fnode8.branch;
    m_free = fnode8.free;
}

template<typename array_t>
Result predecessor_internal(const array_t& keys, const uint64_t x, const internal::fnode8_t& fnode8) {
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

Result FusionNode8::predecessor(const uint64_t* keys, const uint64_t x) const {
    return predecessor_internal(keys, x, { m_mask, m_branch, m_free });
}

Result FusionNode8::predecessor(const SkipAccessor<uint64_t>& keys, const uint64_t x) const {
    return predecessor_internal(keys, x, { m_mask, m_branch, m_free });
}
