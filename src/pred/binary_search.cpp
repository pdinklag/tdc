#include <tdc/pred/binary_search.hpp>
#include <tdc/util/assert.hpp>
#include <tdc/util/likely.hpp>

using namespace tdc::pred;

BinarySearch::BinarySearch(const uint64_t* keys, const size_t num) {
    assert_sorted_ascending(keys, num);
    
    m_min = keys[0];
    m_max = keys[num-1];
}

Result BinarySearch::predecessor(const uint64_t* keys, const size_t num, const uint64_t x) const {
    if(tdc_unlikely(x < m_min))  return Result { false, 0 };
    if(tdc_unlikely(x >= m_max)) return Result { true, num-1 };

    size_t p = 0;
    size_t q = num - 1;

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

    return Result { true, p };
}
