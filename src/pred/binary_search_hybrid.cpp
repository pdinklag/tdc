#include <tdc/pred/binary_search_hybrid.hpp>
#include <tdc/util/assert.hpp>
#include <tdc/util/likely.hpp>

using namespace tdc::pred;

BinarySearchHybrid::BinarySearchHybrid(const uint64_t* keys, const size_t num, const size_t cache_num) : m_cache_num(cache_num) {
    assert_sorted_ascending(keys, num);
    
    m_min = keys[0];
    m_max = keys[num-1];
}

Result BinarySearchHybrid::predecessor_seeded(const uint64_t* keys, size_t p, size_t q, const uint64_t x) const {
    assert(x >= m_min && x < m_max);
    assert(p <= q);
    
    while(q - p > m_cache_num) {
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

    return Result { true, p-1 };
}

Result BinarySearchHybrid::predecessor(const uint64_t* keys, const size_t num, const uint64_t x) const {
    if(tdc_unlikely(x < m_min))  return Result { false, 0 };
    if(tdc_unlikely(x >= m_max)) return Result { true, num-1 };
    return predecessor_seeded(keys, 0, num-1, x);
}
