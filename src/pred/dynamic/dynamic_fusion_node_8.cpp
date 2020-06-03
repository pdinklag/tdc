#include <cassert>

#include <iostream> // FIXME: Debug

#include <tdc/pred/fusion_node_internals.hpp>
#include <tdc/pred/dynamic/dynamic_fusion_node_8.hpp>
#include <tdc/pred/util/packed_byte_array_8.hpp>
#include <tdc/util/likely.hpp>

using namespace tdc::pred::dynamic;

DynamicFusionNode8::DynamicFusionNode8() {
    m_index.x8 = UINT8_MAX; // bkey
}

bool DynamicFusionNode8::used(const size_t j) const {
    return (bkey() & uint8_t(1 << j)) == 0;
}

size_t DynamicFusionNode8::rank(const uint64_t key) const {
    // FIXME: naive, use data structure
    size_t i = 0;
    while(i < size() && m_key[m_index[i]] < key) {
        ++i;
    }
    return i;
}

size_t DynamicFusionNode8::find(const uint64_t key) const {
    // FIXME: naive, use data structure
    for(size_t i = 0; i < size(); i++) {
        if(m_key[m_index[i]] == key) return i;
    }
    return SIZE_MAX;
}

uint64_t DynamicFusionNode8::select(const size_t i) const {
    assert(i < size());
    const size_t j = m_index[i];
    assert(used(j));
    return m_key[j];
}

Result DynamicFusionNode8::predecessor(const uint64_t x) const {
    if(tdc_unlikely(size() == 0)) {
        return { false, 0 };
    } else {
        return internal::predecessor(*this, x, { m_mask, m_branch, m_free });
    }
}

void DynamicFusionNode8::insert(const uint64_t key) {
    assert(size() < 8);

    // find rank
    const size_t i = rank(key);
    assert(i < 8);
    if(i < size()) assert(key != select(i)); // keys must be unique

    // find position to store
    const size_t j = __builtin_ctz(bkey());
    assert(j < 8);
    assert(!used(j));

    // std::cout << "inserting key " << key << " with rank " << i << " in slot " << j << std::endl;

    // update index
    if(size() > 0) {
        for(size_t k = size(); k > i; k--) {
            m_index[k] = m_index[k-1];
        }
    }
    m_index[i] = j;

    // update keys
    m_key[j] = key;
    m_index.x8 &= ~uint8_t(1 << j);
    
    // update data structure
    // FIXME: for now, we update data structure the naive way,
    // meaning that we put the keys in order and build a static fusion node over it
    // any time the trie gets updated
    {
        auto fnode = internal::construct(*this, size());
        m_mask = fnode.mask;
        m_branch = fnode.branch;
        m_free = fnode.free;
    }
}

bool DynamicFusionNode8::remove(const uint64_t key) {
    assert(size() > 0);

    // find key
    const size_t i = find(key);
    if(i < size()) {
        const size_t j = m_index[i];
        assert(j < 8);

        // std::cout << "removing key " << key << " with rank " << i << " from slot " << j << std::endl;
        
        // update index
        for(size_t k = i; k < size() - 1; k++) {
            m_index[k] = m_index[k+1];
        }

        // update keys
        m_index.x8 |= uint8_t(1 << j);
        
        // update data structure
        // FIXME: for now, we update data structure the naive way,
        // meaning that we put the keys in order and build a static fusion node over it
        // any time the trie gets updated
        if(size() > 0) {
            auto fnode = internal::construct(*this, size());
            m_mask = fnode.mask;
            m_branch = fnode.branch;
            m_free = fnode.free;
        }
        
        return true;
    } else {
        // key not found
        return false;
    }
}

void DynamicFusionNode8::print() const {
    std::cout << "DynamicFusionNode8 (size=" << size() << "):" << std::endl;

    for(size_t i = 0; i < size(); i++) {
        const size_t j = m_index[i];
        std::cout << "\ti=" << i << " -> index[i]=" << j << " -> keys[index[i]]=" << m_key[j] << std::endl;
    }
}
