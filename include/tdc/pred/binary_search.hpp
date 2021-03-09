#pragma once

#include <cassert>
#include <cstdint>
#include <cstddef>

#include <tdc/util/concepts.hpp>
#include <tdc/util/likely.hpp>

#include "result.hpp"

namespace tdc {
namespace pred {

/// \brief Simple binary predecessor search.
/// \tparam key_t the key type
template<std::totally_ordered key_t>
class BinarySearch {
public:
    /// \brief Finds the rank of the predecessor of the specified key in the given interval.
    /// \tparam keyarray_t the key array type
    /// \param keys the keys that the compressed trie was constructed for
    /// \param p the left search interval border
    /// \param q the right search interval border
    /// \param x the key in question
    template<IndexAccessTo<key_t> keyarray_t>
    static PosResult predecessor_seeded(const keyarray_t& keys, size_t p, size_t q, const key_t& x)  {
        assert(p <= q);
        while(p < q - 1) {
            assert(x >= keys[p]);
            assert(x < keys[q]);

            const size_t m = (p + q) >> 1ULL;

            const bool le = (keys[m] <= x);

            /*
                the following is a fast form of:
                if(le) p = m; else q = m;
            */
            const size_t le_mask = -size_t(le);
            const size_t gt_mask = ~le_mask;

            if(le) assert(le_mask == SIZE_MAX && gt_mask == 0ULL);
            else   assert(gt_mask == SIZE_MAX && le_mask == 0ULL);

            p = (le_mask & m) | (gt_mask & p);
            q = (gt_mask & m) | (le_mask & q);
        }
        return PosResult { true, p };
    }

    /// \brief Finds the rank of the predecessor of the specified key.
    /// \tparam keyarray_t the key array type
    /// \param keys the keys that the compressed trie was constructed for
    /// \param num the number of keys
    /// \param x the key in question
    template<IndexAccessTo<key_t> keyarray_t>
    static PosResult predecessor(const keyarray_t& keys, const size_t num, const key_t& x)  {
        if(tdc_unlikely(x < keys[0]))  return PosResult { false, 0 };
        if(tdc_unlikely(x >= keys[num-1])) return PosResult { true, num-1 };
        return predecessor_seeded(keys, 0, num-1, x);
    }
    
    /// \brief Finds the rank of the successor of the specified key in the given interval.
    /// \tparam keyarray_t the key array type
    /// \param keys the keys that the compressed trie was constructed for
    /// \param p the left search interval border
    /// \param q the right search interval border
    /// \param x the key in question
    template<IndexAccessTo<key_t> keyarray_t>
    static PosResult successor_seeded(const keyarray_t& keys, size_t p, size_t q, const key_t& x)  {
        assert(p <= q);
        while(p < q - 1) {
            assert(x >= keys[p]);
            assert(x < keys[q]);

            const size_t m = (p + q) >> 1ULL;

            const bool le = (keys[m] <= x);

            /*
                the following is a fast form of:
                if(le) p = m; else q = m;
            */
            const size_t le_mask = -size_t(le);
            const size_t gt_mask = ~le_mask;

            if(le) assert(le_mask == SIZE_MAX && gt_mask == 0ULL);
            else   assert(gt_mask == SIZE_MAX && le_mask == 0ULL);

            p = (le_mask & m) | (gt_mask & p);
            q = (gt_mask & m) | (le_mask & q);
        }
        return PosResult { true, q };
    }

    /// \brief Finds the rank of the successor of the specified key.
    /// \tparam keyarray_t the key array type
    /// \param keys the keys that the compressed trie was constructed for
    /// \param num the number of keys
    /// \param x the key in question
    template<IndexAccessTo<key_t> keyarray_t>
    static PosResult successor(const keyarray_t& keys, const size_t num, const key_t& x)  {
        if(tdc_unlikely(x <= keys[0]))  return PosResult { true, 0 };
        if(tdc_unlikely(x > keys[num-1])) return PosResult { false, 0 };
        return successor_seeded(keys, 0, num-1, x);
    }
};

}} // namespace tdc::pred
