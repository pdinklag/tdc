#pragma once

#include <iterator>

namespace tdc {
namespace vec {

template<typename itemref_t>
class Iterator {
protected:
    itemref_t m_ref;

public:
    // declarations for std::iterator_traits
    using difference_type = void;
    using value_type = itemref_t;
    using pointer = itemref_t*;
    using reference = itemref_t&;
    using iterator_category = std::bidirectional_iterator_tag;

    Iterator() = delete;
    
    inline Iterator(itemref_t ref) : m_ref(ref) {
    }
    
    Iterator(const Iterator& other) = default;
    Iterator(Iterator&& other) = default;
    Iterator& operator=(const Iterator&) = default;
    Iterator& operator=(Iterator&&) = default;
    
    /// \brief Prefix increment.
    inline Iterator& operator++() {
        ++m_ref;
        return *this;
    }

    /// \brief Postfix increment.
    inline Iterator operator++(int) {
        Iterator before(*this);
        ++*this;
        return before;
    }

    /// \brief Prefix decrement.
    inline Iterator& operator--() {
        --m_ref;
        return *this;
    }

    /// \brief Postfix decrement.
    inline Iterator operator--(int) {
        Iterator before(*this);
        --*this;
        return before;
    }
    
    /// \brief Equality test.
    inline bool operator==(const Iterator& other) const {
        return m_ref == other.m_ref;
    }
    
    /// \brief Inequality test.
    inline bool operator!=(const Iterator& other) const {
        return m_ref != other.m_ref;
    }
    
    /// \brief Dereference.
    inline itemref_t& operator*() {
        return m_ref;
    }

    /// \brief Dereference.
    inline const itemref_t& operator*() const {
        return m_ref;
    }
    
    /// \brief Dereference.
    inline itemref_t* operator->() {
        return &m_ref;
    }
    
    /// \brief Dereference.
    inline const itemref_t* operator->() const {
        return &m_ref;
    }
};

}} // namespace tdc::vec

/*
#include <iterator>

namespace std {

template<typename itemref_t>
struct iterator_traits<tdc::vec::Iterator<itemref_t>> {
    
};

} // namespace std
*/
