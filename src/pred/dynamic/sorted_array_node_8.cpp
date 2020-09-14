#include <tdc/pred/dynamic/btree/sorted_array_node_8.hpp>

#include <cassert>
#include <tdc/util/likely.hpp>

using namespace tdc::pred::dynamic;

SortedArrayNode8::SortedArrayNode8() : m_size(0) {
}

Result SortedArrayNode8::predecessor(const uint64_t x) const {
    if(tdc_unlikely(x < m_keys[0]))  return Result { false, 0 };
    if(tdc_unlikely(x >= m_keys[m_size-1])) return Result { true, m_size-1ULL };
    
    size_t i = 1;
    while(m_keys[i] <= x) ++i;
    return Result { true, i-1 };
}

void SortedArrayNode8::insert(const uint64_t key) {
    assert(m_size < 8);
    size_t i = 0;
    while(i < m_size && m_keys[i] < key) ++i;
    for(size_t j = m_size; j > i; j--) m_keys[j] = m_keys[j-1];
    m_keys[i] = key;
    ++m_size;
    
    #ifndef NDEBUG
    for(size_t i = 0; i < m_size-1; i++) {
        assert(m_keys[i] < m_keys[i+1]);
    }
    #endif
}

bool SortedArrayNode8::remove(const uint64_t key) {
    assert(m_size > 0);
    for(size_t i = 0; i < m_size; i++) {
        if(m_keys[i] == key) {
            for(size_t j = i; j < m_size-1; j++) m_keys[j] = m_keys[j+1];
            --m_size;
            #ifndef NDEBUG
            if(m_size > 0) {
                for(size_t i = 0; i < m_size-1; i++) {
                    assert(m_keys[i] < m_keys[i+1]);
                }
            }
            #endif
            return true;
        }
    }
    return false;
}
