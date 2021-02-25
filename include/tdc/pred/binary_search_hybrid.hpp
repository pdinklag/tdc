#pragma once

#include <cassert>
#include <cstdint>
#include <cstddef>

#include <tdc/util/concepts.hpp>

#include "result.hpp"

namespace tdc {
namespace pred {

/// \brief Binary predecessor search that switches to linear search in small intervals.
/// \tparam key_t the key type
/// \tparam linear_threshold if the search interval becomes smaller than this, switch to linear search 
template<std::totally_ordered key_t, size_t linear_threshold = 512ULL / sizeof(key_t)>
class BinarySearchHybrid {
public:
    /// \brief Finds the rank of the predecessor of the specified key in the given interval.
    /// \tparam keyarray_t the key array type
    /// \param keys the keys that the compressed trie was constructed for
    /// \param p the left search interval border
    /// \param q the right search interval border
    /// \param x the key in question
    template<IndexAccessTo<key_t> keyarray_t>
    static PosResult predecessor_seeded(const keyarray_t& keys, size_t p, size_t q, const key_t& x) {
        assert(p <= q);
        
        while(q - p > linear_threshold) {
            assert(x >= keys[p]);

            const size_t m = (p + q) >> 1ULL;

            const bool le = (keys[m] <= x);

            /*
                the following is a fast form of:
                if(le) p = m; else q = m;
            */
            const size_t le_mask = -size_t(le);
            const size_t gt_mask = ~le_mask;

            p = (le_mask & m) | (gt_mask & p);
            q = (gt_mask & m) | (le_mask & q);
        }

        // linear search
        while(keys[p] <= x) ++p;
        assert(keys[p-1] <= x);

        return PosResult { true, p-1 };
    }
    
    /// \brief Finds the rank of the predecessor of the specified key.
    /// \tparam keyarray_t the key array type
    /// \param keys the keys that the compressed trie was constructed for
    /// \param num the number of keys
    /// \param x the key in question
    template<IndexAccessTo<key_t> keyarray_t>
    static PosResult predecessor(const keyarray_t& keys, const size_t num, const key_t& x) {
        if(tdc_unlikely(x < keys[0]))  return PosResult { false, 0 };
        if(tdc_unlikely(x >= keys[num-1])) return PosResult { true, num-1 };
        return predecessor_seeded(keys, 0, num-1, x);
    }
};

}} // namespace tdc::pred
