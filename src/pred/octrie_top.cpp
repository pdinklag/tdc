#include <tdc/pred/octrie_top.hpp>

#include <algorithm>
#include <cassert>

#include <iostream> // FIXME: DEBUG

using namespace tdc::pred;

OctrieTop::OctrieTop(const uint64_t* keys, const size_t num, const size_t cut_levels)
    : Octrie(keys, num, std::max(log8_ceil(num), size_t(cut_levels + 1)) - cut_levels) {

    m_cut_levels = cut_levels;
    m_full_octree_size_ub = octree_size(m_full_octree_height);
    m_search_interval = eight_to_the(cut_levels);
    m_search = BinarySearchHybrid(keys, num);
}

PosResult OctrieTop::predecessor(const uint64_t* keys, const size_t num, const uint64_t x) const {
    auto r = Octrie::predecessor(keys, num, x);
    if(!r.exists) {
        return r;
    }
    
    // std::cout << "prdecessor node on level " << m_height << " is " << r.pos << std::endl;
    
    size_t node = r.pos + m_octree_size_ub;
    for(size_t j = 0; j < m_cut_levels; j++) {
        node = 8 * node + 1;
    }
    
    const size_t p = node - m_full_octree_size_ub;
    const size_t q = std::min(p + m_search_interval, num);
    return m_search.predecessor_seeded(keys, p, q, x);
}
