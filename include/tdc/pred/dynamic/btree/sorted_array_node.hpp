#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>

#include <tdc/pred/result.hpp>
#include <tdc/util/likely.hpp>

namespace tdc {
namespace pred {
namespace dynamic {

using Result = ::tdc::pred::Result;

template<typename key_t, size_t m_capacity>
class SortedArrayNode {
private:
    static_assert(m_capacity < 256);

    key_t m_keys[m_capacity];
    uint8_t m_size;

public:
    /// \brief Constructs an empty fusion node.
    SortedArrayNode(): m_size(0) {
    }

    SortedArrayNode(const SortedArrayNode&) = default;
    SortedArrayNode(SortedArrayNode&&) = default;

    SortedArrayNode& operator=(const SortedArrayNode&) = default;
    SortedArrayNode& operator=(SortedArrayNode&&) = default;

    /// \brief Accesses the element with given rank
    /// \param i the rank in question    
    inline uint64_t operator[](const size_t i) const {
        return m_keys[i];
    }
    
    /// \brief Finds the rank of the predecessor of the specified key in the node.
    /// \param x the key in question
    Result predecessor(const key_t x) const {
        if(tdc_unlikely(x < m_keys[0]))  return Result { false, 0 };
        if(tdc_unlikely(x >= m_keys[m_size-1])) return Result { true, m_size - 1ULL };
        
        size_t i = 1;
        while(m_keys[i] <= x) ++i;
        return Result { true, i-1 };
    }

    /// \brief Inserts the specified key.
    /// \param key the key to insert
    void insert(const key_t key) {
        assert(m_size < m_capacity);
        size_t i = 0;
        while(i < m_size && m_keys[i] < key) ++i;
        for(size_t j = m_size; j > i; j--) m_keys[j] = m_keys[j-1];
        m_keys[i] = key;
        ++m_size;
    }

    /// \brief Removes the specified key.
    /// \param key the key to remove
    /// \return whether the item was found and removed
    bool remove(const key_t key) {
        assert(m_size > 0);
        for(size_t i = 0; i < m_size; i++) {
            if(m_keys[i] == key) {
                for(size_t j = i; j < m_size-1; j++) m_keys[j] = m_keys[j+1];
                --m_size;
                return true;
            }
        }
        return false;
    }

    /// \brief Returns the current size of the fusion node.
    inline size_t size() const {
        return m_size;
    }
} __attribute__((__packed__));

}}} // namespace tdc::pred::dynamic
