#include <tdc/pred/octrie.hpp>

#include <algorithm>
#include <iostream> // FIXME: DEBUG

#include <tdc/math/idiv.hpp>
#include <tdc/util/assert.hpp>
#include <tdc/util/skip_accessor.hpp>

using namespace tdc::pred;

Octrie::Octrie(const uint64_t* keys, const size_t num, const size_t max_height) {
    assert(num > 0);
    assert_sorted_ascending(keys, num);
    
    // allocate memory for octree
    m_full_octree_height = log8_ceil(num);
    assert(max_height <= m_full_octree_height);
    
    m_height = max_height;
    m_octree_size_ub = octree_size(m_height); 
    
    // std::cout << "num=" << num << ", height=" << m_height << ", octree_size <= " << m_octree_size_ub << std::endl;
    
    m_octree.resize(m_height);
    
    // construct octree bottom-up
    for(size_t l = 0; l < m_height; l++) {
        const size_t level = m_height - l - 1;

        // we want to sample every k-th key, with k=1 for the last level, k=8 for the level above, k=64 for the level above that, ...
        const size_t k = eight_to_the(l + m_full_octree_height - m_height);
        
        auto& octree_level = m_octree[level];
        octree_level.first_node = (level > 0) ? octree_size(level) : 0;
        octree_level.nodes.reserve(math::idiv_ceil(num, 8 * k));

        // scan keys
        size_t i = 0;
        while(i < num) {
            // (virtually) sample the next at most 8 keys
            const size_t j = std::min(math::idiv_ceil(num - i, k), uint64_t(8));
            
            SkipAccessor<uint64_t> sample(keys, k, i);
            i += j * k;
            
            // construct a compressed trie for the sample and put it in the octree
            assert(octree_level.nodes.size() < octree_level.nodes.capacity());
            octree_level.nodes.emplace_back(sample, j);
        }

        assert(octree_level.nodes.size() == octree_level.nodes.capacity());
    }
    
    m_root = &m_octree[0].nodes[0];
    assert(m_root);
}

Octrie::Octrie(const uint64_t* keys, const size_t num) : Octrie(keys, num, log8_ceil(num)) {
}

Result Octrie::predecessor(const uint64_t* keys, const size_t num, const uint64_t x) const {
    // std::cout << "predecessor(" << x << ")" << std::endl;
    
    size_t k = eight_to_the(m_full_octree_height - 1); // sample distance
    size_t i = 0; // sample offset
    
    // first, check if there is a predecessor in the root node
    Result r = m_root->predecessor(SkipAccessor<uint64_t>(keys, k, i), x);
    if(!r.exists) return r; // if not, there is no predecessor at all
    
    // std::cout << "root predecessor is " << r.pos << std::endl;
    size_t node = r.pos + 1; // start at the corresponding child
    size_t level = 1;
    while(level < m_height) {
        i += r.pos * k;
        k /= 8;
        assert(k);
        
        const auto& octree_level = m_octree[level];
        r = octree_level.nodes[node - octree_level.first_node].predecessor(SkipAccessor<uint64_t>(keys, k, i), x); // find predecessor in node
        // std::cout << "node " << node << " predecessor is " << r.pos << std::endl;
        assert(r.exists);
        
        // descend to child
        node = 8 * node + 1 + r.pos;
        ++level;
    }
    
    // compute position in original input
    return Result { true, node - m_octree_size_ub };
}
