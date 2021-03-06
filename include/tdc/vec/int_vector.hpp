#pragma once
    
#include <cstring>
#include <memory>
#include <utility>

#include "allocate.hpp"
#include "item_ref.hpp"
#include "iterator.hpp"
#include "vector_builder.hpp"

#include <tdc/math/bit_mask.hpp>
#include <tdc/math/idiv.hpp>

namespace tdc {
namespace vec {

class IntVectorBuilder; // fwd

/// \brief A vector of integers of arbitrary bit width, using bit packing to minimize the required space.
///
/// Int vectors are static, i.e., integers cannot be inserted or deleted.
class IntVector {
public:
    /// \brief The \ref VectorBuilder type for integer vectors.
    using builder_type = IntVectorBuilder;

private:
    friend class ItemRef<IntVector, uint64_t>;
    friend class ConstItemRef<IntVector, uint64_t>;

    size_t m_size;
    size_t m_width;
    size_t m_mask;
    std::unique_ptr<uint64_t[]> m_data;

    uint64_t get(const size_t i) const;
    void set(const size_t i, const uint64_t v);

public:
    /// \brief Proxy for reading and writing a single integer.
    using IntRef = ItemRef<IntVector, uint64_t>;
    
    /// \brief Proxy for reading a single integer.
    using ConstIntRef = ConstItemRef<IntVector, uint64_t>;

    /// \brief Constructs an empty integer vector of zero length and width.
    inline IntVector() : m_size(0), m_width(0), m_mask(0) {
    }

    /// \brief Constructs an integer vector with the specified length and width.
    /// \param size the number of integers
    /// \param width the width of each integer in bits
    /// \param initialize if \c true, all values will be initialized with zero
    inline IntVector(const size_t size, const size_t width, const bool initialize = true) {
        m_size = size;
        m_width = width;
        m_mask = math::bit_mask<size_t>(width);
        m_data = allocate_integers(size, width, initialize);
    }

    inline IntVector(const IntVector& other) { *this = other; }
    IntVector(IntVector&& other) = default;

    inline IntVector& operator=(const IntVector& other) {
        m_size = other.m_size;
        m_width = other.m_width;
        m_mask = other.m_mask;
        m_data = allocate_integers(m_size, m_width, false);
        memcpy(m_data.get(), other.m_data.get(), math::idiv_ceil(m_size * m_width, 64ULL) * sizeof(uint64_t));
        return *this;
    }

    IntVector& operator=(IntVector&& other) = default;
    
    /// \brief Exchanges the contents of this vector with that of the other.
    ///
    /// \param other the other vector
    inline void swap(IntVector& other) {
        std::swap(m_size, other.m_size);
        std::swap(m_width, other.m_width);
        std::swap(m_mask, other.m_mask);
        std::swap(m_data, other.m_data);
    }

    /// \brief Resizes the integer vector with the specified new length and width.
    ///
    /// \param size the new number of integers
    /// \param width the new width of each integer in bits
    void resize(const size_t size, const size_t width);

    /// \brief Resizes the integer vector with the specified new length and current bit width.
    ///
    /// \param size the new number of integers
    inline void resize(const size_t size) {
        resize(size, m_width);
    }

    /// \brief Reads the specified integer.
    /// \param i the number of the integer to read
    inline uint64_t operator[](const size_t i) const {
        return get(i);
    }

    /// \brief Accesses the specified integer.
    /// \param i the number of the integer to access
    inline IntRef operator[](const size_t i) {
        return IntRef(*this, i);
    }
    
    /// \brief Accesses the first integer.
    inline uint64_t front() const {
        return get(0);
    }
    
    /// \brief Accesses the first integer.
    inline IntRef front() {
        return IntRef(*this, 0);
    }
    
    /// \brief Accesses the last integer.
    inline uint64_t back() const {
        return get(m_size-1);
    }
    
    /// \brief Accesses the last integer.
    inline IntRef back() {
        return IntRef(*this, m_size-1);
    }

    /// \brief The width of each integer in bits.
    inline size_t width() const {
        return m_width;
    }
    
    /// \brief The number of integers in the vector.
    inline size_t size() const {
        return m_size;
    }
    
    /// \brief STL-like iterator to the beginning of the vector.
    inline Iterator<IntRef> begin() {
        return Iterator<IntRef>(IntRef(*this, 0));
    }
    
    /// \brief STL-like iterator to the i-th element of the vector.
    inline Iterator<IntRef> at(const size_t i) {
        return Iterator(IntRef(*this, i));
    }
    
    /// \brief STL-like iterator to the end of the vector.
    inline Iterator<IntRef> end() {
        return Iterator<IntRef>(IntRef(*this, m_size));
    }
    
    /// \brief STL-like const iterator to the beginning of the vector.
    inline Iterator<ConstIntRef> begin() const {
        return Iterator(ConstIntRef(*this, 0));
    }
    
    /// \brief STL-like iterator to the i-th element of the vector.
    inline Iterator<ConstIntRef> at(const size_t i) const {
        return Iterator(ConstIntRef(*this, i));
    }
    
    /// \brief STL-like const iterator to the end of the vector.
    inline Iterator<ConstIntRef> end() const {
        return Iterator(ConstIntRef(*this, m_size));
    }
};

/// \brief Specialization of vector builders for \ref IntVector.
class IntVectorBuilder : public VectorBuilder<IntVector> {
public:
    IntVectorBuilder(const size_t width, const size_t capacity = 0);

    using VectorBuilder::VectorBuilder;
};

}} // namespace tdc::vec
