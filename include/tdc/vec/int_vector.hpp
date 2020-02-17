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

    size_t m_size;
    size_t m_width;
    size_t m_mask;
    std::unique_ptr<uint64_t[]> m_data;

    inline std::unique_ptr<uint64_t[]> allocate(const size_t num, const size_t w) {
        const size_t num64 = div64_ceil(num * w);
        uint64_t* p = new uint64_t[num64];
        memset(p, 0, num64 * sizeof(uint64_t));
        return std::unique_ptr<uint64_t[]>(p);
    }

    inline void set(size_t i, uint64_t v) {
        v &= m_mask; // make sure it fits...
        
        const size_t j = i * m_width;
        const size_t a = j >> 6ULL;       // left border
        const size_t b = (j + m_width - 1ULL) >> 6ULL; // right border
        if(a < b) {
            // the bits are the suffix of m_data[a] and prefix of m_data[b]
            const size_t da = j & 63ULL;
            const size_t wa = 64ULL - da;
            const size_t wb = m_width - wa;
            const size_t db = 64ULL - wb;

            // combine the da lowest bits from a and the wa lowest bits of v
            const uint64_t a_lo = m_data[a] & bit_mask(da);
            const uint64_t v_lo = v & bit_mask(wa);
            m_data[a] = (v_lo << da) | a_lo;

            // combine the db highest bits of b and the wb highest bits of v
            const uint64_t b_hi = m_data[b] >> wb;
            const uint64_t v_hi = v >> wa;
            m_data[b] = (b_hi << wb) | v_hi;
        } else {
            const size_t dl = j & 63ULL;
            const size_t dvl = dl + m_width;
            
            const uint64_t xa = m_data[a];
            const uint64_t lo = xa & bit_mask(dl);
            const uint64_t hi = xa >> dvl;
            m_data[a] = lo | (v << dl) | (hi << dvl);
        }
    }

    inline uint64_t get(size_t i) const {
        const size_t j = i * m_width;
        const size_t a = j >> 6ULL;                    // left border
        const size_t b = (j + m_width - 1ULL) >> 6ULL; // right border

        // da is the distance of a's relevant bits from the left border
        const size_t da = j & 63ULL;

        // wa is the number of a's relevant bits
        const size_t wa = 64ULL - da;

        // get the wa highest bits from a
        const uint64_t a_hi = m_data[a] >> da;

        // get b (its high bits will be masked away below)
        // NOTE: we could save this step if we knew a == b,
        //       but the branch caused by checking that is too expensive
        const uint64_t b_lo = m_data[b];

        // combine
        return ((b_lo << wa) | a_hi) & m_mask;
    }

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

    /// \brief Rebuilds the integer vector with the specified new length and width.
    ///
    /// \param size the new number of integers
    /// \param width the new width of each integer in bits
    inline void rebuild(size_t size, size_t width) {
        IntVector new_iv(size, width);
        for(size_t i = 0; i < size; i++) {
            new_iv.set(i, get(i));
        }
        
        m_size = new_iv.m_size;
        m_width = new_iv.m_width;
        m_mask = new_iv.m_mask;
        m_data = std::move(new_iv.m_data);
    }

    /// \brief Rebuilds the integer vector with the specified new length and current bit width.
    ///
    /// \param size the new number of integers
    inline void rebuild(size_t size) {
        rebuild(size, m_width);
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
