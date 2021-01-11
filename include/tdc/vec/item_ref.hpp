#pragma once

namespace tdc {
namespace vec {

/// \brief Proxy for reading and writing a single item in a vector.
/// \tparam vector_t the vector type
/// \tparam item_t the item type
template<typename vector_t, typename item_t>
class ItemRef {
protected:
    vector_t* m_vec;
    size_t m_i;
    
public:
    inline ItemRef() : m_vec(nullptr) {
    };

    /// \brief Main constructor.
    /// \param vec the vector this proxy belongs to.
    /// \param i the number of the referred item.
    inline ItemRef(vector_t& vec, size_t i) : m_vec(&vec), m_i(i) {
    }
    
    ItemRef(const ItemRef& other) = default;
    ItemRef(ItemRef&& other) = default;
    
    ItemRef& operator=(const ItemRef&) = delete; // ambiguity with value assignment - use explicit casts!
    ItemRef& operator=(ItemRef&&) = delete; // ambiguity with value assignment - use explicit casts!
    
    /// \brief Explicit copy assignment.
    inline void copy_assign(const ItemRef& other) {
        m_vec = other.m_vec;
        m_i = other.m_i;
    }
    
    /// \brief Explicit move assignment.
    inline void move_assign(ItemRef&& other) {
        m_vec = other.m_vec;
        m_i = other.m_i;
    }

    /// \brief Reads the referred item.
    inline operator item_t() const {
        return m_vec->get(m_i);
    }

    /// \brief Writes the referred integer.
    /// \param v the value to write
    inline void operator=(item_t v) const {
        m_vec->set(m_i, v);
    }
    
    /// \brief Prefix increment.
    inline ItemRef& operator++() {
        ++m_i;
        return *this;
    }

    /// \brief Postfix increment.
    inline ItemRef operator++(int) {
        ItemRef before(*this);
        ++*this;
        return before;
    }

    /// \brief Prefix decrement.
    inline ItemRef& operator--() {
        --m_i;
        return *this;
    }

    /// \brief Postfix decrement.
    inline ItemRef operator--(int) {
        ItemRef before(*this);
        --*this;
        return before;
    }
    
    /// \brief Equality test.
    inline bool operator==(const ItemRef& other) const {
        return m_vec == other.m_vec && m_i == other.m_i;
    }
    
    /// \brief Inequality test.
    inline bool operator!=(const ItemRef& other) const {
        return m_vec != other.m_vec || m_i != other.m_i;
    }
};

/// \brief Proxy for reading a single item in a vector.
/// \tparam vector_t the vector type
/// \tparam item_t the item type
template<typename vector_t, typename item_t>
class ConstItemRef {
protected:
    const vector_t* m_vec;
    size_t m_i;
    
public:
    inline ConstItemRef() : m_vec(nullptr) {
    };

    /// \brief Main constructor.
    /// \param vec the vector this proxy belongs to.
    /// \param i the number of the referred item.
    inline ConstItemRef(const vector_t& vec, size_t i) : m_vec(&vec), m_i(i) {
    }
    
    ConstItemRef(const ConstItemRef& other) = default;
    ConstItemRef(ConstItemRef&& other) = default;
    
    ConstItemRef& operator=(const ConstItemRef&) = delete; // ambiguity with value assignment - consistency with ItemRef
    ConstItemRef& operator=(ConstItemRef&&) = delete; // ambiguity with value assignment - consistency with ItemRef

    /// \brief Explicit copy assignment.
    inline void copy_assign(const ConstItemRef& other) {
        m_vec = other.m_vec;
        m_i = other.m_i;
    }
    
    /// \brief Explicit move assignment.
    inline void move_assign(ConstItemRef&& other) {
        m_vec = other.m_vec;
        m_i = other.m_i;
    }

    /// \brief Reads the referred item.
    inline operator item_t() const {
        return m_vec->get(m_i);
    }

    /// \brief Prefix increment.
    inline ConstItemRef& operator++() {
        ++m_i;
        return *this;
    }

    /// \brief Postfix increment.
    inline ConstItemRef operator++(int) {
        ConstItemRef before(*this);
        ++*this;
        return before;
    }

    /// \brief Prefix decrement.
    inline ConstItemRef& operator--() {
        --m_i;
        return *this;
    }

    /// \brief Postfix decrement.
    inline ConstItemRef operator--(int) {
        ConstItemRef before(*this);
        --*this;
        return before;
    }
    
    /// \brief Equality test.
    inline bool operator==(const ConstItemRef& other) const {
        return m_vec == other.m_vec && m_i == other.m_i;
    }
    
    /// \brief Inequality test.
    inline bool operator!=(const ConstItemRef& other) const {
        return m_vec != other.m_vec || m_i != other.m_i;
    }
};

}} // namespace tdc::vec
