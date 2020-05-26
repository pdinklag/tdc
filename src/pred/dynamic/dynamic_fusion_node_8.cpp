#include <cassert>

#include <iostream> // FIXME: Debug

#include <tdc/pred/fusion_node_internals.hpp>
#include <tdc/pred/dynamic/dynamic_fusion_node_8.hpp>
#include <tdc/pred/util/packed_byte_array_8.hpp>
#include <tdc/util/likely.hpp>

using namespace tdc::pred::dynamic;

DynamicFusionNode8::DynamicFusionNode8() : m_size(0), m_bkey(UINT8_MAX) {
}

bool DynamicFusionNode8::used(const size_t j) const {
    return (m_bkey & uint8_t(1 << j)) == 0;
}

size_t DynamicFusionNode8::rank(const uint64_t key) const {
    // FIXME: naive, use data structure
    auto index = (PackedByteArray8)m_index;

    size_t i = 0;
    while(i < m_size && m_key[index.u8[i]] < key) {
        ++i;
    }
    return i;
}

size_t DynamicFusionNode8::find(const uint64_t key) const {
    // FIXME: naive, use data structure
    auto index = (PackedByteArray8)m_index;

    for(size_t i = 0; i < m_size; i++) {
        if(m_key[index.u8[i]] == key) return i;
    }
    return SIZE_MAX;
}

uint64_t DynamicFusionNode8::select(const size_t i) const {
    assert(i < m_size);
    auto index = (PackedByteArray8)m_index;
    const size_t j = index.u8[i];
    assert(used(j));
    return m_key[j];
}

Result DynamicFusionNode8::predecessor(const uint64_t x) const {
    if(tdc_unlikely(m_size == 0)) {
        return { false, 0 };
    } else {
        return internal::predecessor(*this, x, { m_mask, m_branch, m_free });
    }
}

void DynamicFusionNode8::insert(const uint64_t key) {
    assert(m_size < 8);

    // find rank
    const size_t i = rank(key);
    assert(i < 8);
    if(i < m_size) assert(key != select(i)); // keys must be unique

    // find position to store
    const size_t j = __builtin_ctz(m_bkey);
    assert(j < 8);
    assert(!used(j));

    std::cout << "inserting key " << key << " with rank " << i << " in slot " << j << std::endl;

    // update index
    auto index = (PackedByteArray8)m_index;
    if(m_size > 0) {
        for(size_t k = m_size; k > i; k--) {
            index.u8[k] = index.u8[k-1];
        }
    }
    index.u8[i] = j;
    m_index = index.u64;

    // update keys
    m_key[j] = key;
    m_bkey &= ~uint8_t(1 << j);

    // increase size
    ++m_size;
    
    // update data structure
    // FIXME: for now, we update data structure the naive way,
    // meaning that we put the keys in order and build a static fusion node over it
    // any time the trie gets updated
    {
        auto fnode = internal::construct(*this, m_size);
        m_mask = fnode.mask;
        m_branch = fnode.branch;
        m_free = fnode.free;
    }
}

void DynamicFusionNode8::remove(const uint64_t key) {
    assert(m_size > 0);

    // find key
    const size_t i = find(key);
    if(i < m_size) {
        auto index = (PackedByteArray8)m_index;
        const size_t j = index.u8[i];
        assert(j < 8);

        std::cout << "removing key " << key << " with rank " << i << " from slot " << j << std::endl;
        
        // update index
        for(size_t k = i; k < m_size - 1; k++) {
            index.u8[k] = index.u8[k+1];
        }
        m_index = index.u64;

        // update keys
        m_bkey |= uint8_t(1 << j);

        // decrease size
        --m_size;
        
        // update data structure
        // FIXME: for now, we update data structure the naive way,
        // meaning that we put the keys in order and build a static fusion node over it
        // any time the trie gets updated
        if(m_size > 0) {
            auto fnode = internal::construct(*this, m_size);
            m_mask = fnode.mask;
            m_branch = fnode.branch;
            m_free = fnode.free;
        }
    } else {
        // key not found, do nothing
    }
}

void DynamicFusionNode8::print() const {
    std::cout << "DynamicFusionNode8 (size=" << m_size << "):" << std::endl;

    auto index = (PackedByteArray8)m_index;
    for(size_t i = 0; i < m_size; i++) {
        const size_t j = index.u8[i];
        std::cout << "\ti=" << i << " -> index[i]=" << j << " -> keys[index[i]]=" << m_key[j] << std::endl;
    }
}
