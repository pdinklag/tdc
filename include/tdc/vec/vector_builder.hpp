#pragma once

#include <cassert>
#include <cstddef>
#include <utility>

namespace tdc {
namespace vec {

/// \brief Adds appending and removing functionality to a vector.
///
/// The vector types in this namespace are generally static.
/// However, it is often useful to fill it sequentially, especially when its final size is not known in advance.
/// This class adds the ability to push items to the back of the vector, growing it using the capacity doubling strategy.
/// Once the vector is fully built up, \ref finalize should be used to drop the appending functionality.
///
/// Usually, you do not construct an object of this directly but use static providers from the vector types that support it.
///
/// \tparam vector_t the underlying vector type
template<typename vector_t>
class VectorBuilder {
protected:
    vector_t m_vector;

private:
    size_t   m_size;

public:
    /// \brief Constructs the vector.
    /// \param capacity the vector's initial capacity
    VectorBuilder(const size_t capacity = 0) : m_size(0) {
        if(capacity > 0) {
            m_vector.resize(capacity);
        }
    }

    /// \brief Copy constructor.
    /// \param other the object to copy
    inline VectorBuilder(const VectorBuilder& other) {
        *this = other;
    }

    /// \brief Move constructor.
    /// \param other the object to move
    inline VectorBuilder(VectorBuilder&& other) {
        *this = std::move(other);
    }

    /// \brief Copy assignment.
    /// \param other the object to copy
    inline VectorBuilder& operator=(const VectorBuilder& other) {
        m_vector = other.m_vector;
        m_size = other.m_size;
        return *this;
    }

    /// \brief Move assignment.
    /// \param other the object to move
    inline VectorBuilder& operator=(VectorBuilder&& other) {
        m_vector = std::move(other.m_vector);
        m_size = other.m_size;
        return *this;
    }

    /// \brief Appends an item to the end of the vector.
    /// \tparam item_t the item type
    /// \param item the item to append
    template<typename item_t>
    void push_back(const item_t& item) {
        const size_t cap = m_vector.size();
        if(m_size >= capacity()) {
            // grow by capacity doubling
            m_vector.resize(2 * cap);
        }

        m_vector[m_size++] = item;
    }

    /// \brief Removes and returns the last element from the vector.
    auto pop_back() {
        assert(m_size > 0);
        return m_vector[--m_size];        
    }

    /// \brief Shrinks the vector's capacity to match its size.
    void shrink_to_fit() {
        if(m_vector.size() > m_size) {
            m_vector.resize(m_size);
        }
    }

    /// \brief Finalizes the construction process and returns an rvalue reference to the underlying vector for storage.
    /// \param shrink if \c true, \ref shrink_to_fit will be called before returning
    vector_t finalize(bool shrink = true) {
        if(shrink) {
            shrink_to_fit();
        }
        return m_vector;
    }

    /// \brief Gets an element from the vector.
    /// \param i the index of the element
    inline auto operator[](const size_t i) const {
        return m_vector[i];
    }

    /// \brief Accesses an element from the vector.
    /// \param i the index of the element
    inline auto operator[](const size_t i) {
        return m_vector[i];
    }

    /// \brief Returns the size of the vector, that is, the number of elements currently in it.
    inline size_t size() const {
        return m_size;
    }

    /// \brief Returns the capacity of the vector, that is, the number of elements allocated.
    inline size_t capacity() const {
        return m_vector.size();
    }
};

}} // namespace tdc::vec
