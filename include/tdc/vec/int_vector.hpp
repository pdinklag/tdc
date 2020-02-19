#pragma once
    
#include <cstring>
#include <memory>
#include <utility>

#include "util.hpp"

namespace tdc {
namespace vec {

/// \brief A vector of integers of arbitrary bit width, using bit packing to minimize the required space.
///
/// Int vectors are static, i.e., integers cannot be inserted or deleted.
class IntVector {
private:
    inline static constexpr uint64_t bit_mask(const uint64_t bits) {
        return (1ULL << bits) - 1ULL;
    }

    static std::unique_ptr<uint64_t[]> allocate(const size_t num, const size_t w);

    size_t m_size;
    size_t m_width;
    size_t m_mask;
    std::unique_ptr<uint64_t[]> m_data;

    uint64_t get(const size_t i) const;
    void set(const size_t i, const uint64_t v);

public:
    /// \brief Proxy for reading and writing a single integer.
    struct IntRef {
        /// \brief The integer vector this proxy belongs to.
        IntVector* iv;

        /// \brief The number of the referred integer.
        size_t i;

        /// \brief Reads the referred integer.
        inline operator uint64_t() const {
            return iv->get(i);
        }

        /// \brief Writes the referred integer.
        /// \param v the value to write
        inline void operator=(uint64_t v) {
            iv->set(i, v);
        }
    };

    /// \brief Constructs an empty integer vector of zero length and width.
    inline IntVector() : m_size(0), m_width(0), m_mask(0) {
    }

    /// \brief Copy constructor.
    /// \param other the integer vector to copy
    inline IntVector(const IntVector& other) {
        *this = other;
    }

    /// \brief Move constructor.
    /// \param other the integer vector to move
    inline IntVector(IntVector&& other) {
        *this = std::move(other);
    }

    /// \brief Constructs an integer vector with the specified length and width.
    /// \param size the number of integers
    /// \param width the width of each integer in bits
    inline IntVector(size_t size, size_t width) {
        m_size = size;
        m_width = width;
        m_mask = bit_mask(width);
        m_data = allocate(size, width);
    }

    /// \brief Copy assignment.
    /// \param other the integer vector to copy
    inline IntVector& operator=(const IntVector& other) {
        m_size = other.m_size;
        m_width = other.m_width;
        m_mask = other.m_mask;
        m_data = allocate(m_size, m_width);
        memcpy(m_data.get(), other.m_data.get(), div64_ceil(m_size * m_width));
        return *this;
    }

    /// \brief Move assignment.
    /// \param other the integer vector to move
    inline IntVector& operator=(IntVector&& other) {
        m_size = other.m_size;
        m_width = other.m_width;
        m_mask = other.m_mask;
        m_data = std::move(other.m_data);
        return *this;
    }

    /// \brief Resizes the integer vector with the specified new length and width.
    ///
    /// \param size the new number of integers
    /// \param width the new width of each integer in bits
    void resize(const size_t size, const size_t width);

    /// \brief Resizes the integer vector with the specified new length and current bit width.
    ///
    /// \param size the new number of integers
    inline void resize(size_t size) {
        resize(size, m_width);
    }

    /// \brief Reads the specified integer.
    /// \param i the number of the integer to read
    inline uint64_t operator[](size_t i) const {
        return get(i);
    }

    /// \brief Accesses the specified integer.
    /// \param i the number of the integer to access
    inline IntRef operator[](size_t i) {
        return IntRef { this, i };
    }

    /// \brief The width of each integer in bits.
    inline size_t width() const {
        return m_width;
    }
    
    /// \brief The number of integers in the vector.
    inline size_t size() const {
        return m_size;
    }
};

}} // namespace tdc::vec
