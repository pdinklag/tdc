#pragma once
    
#include <algorithm>
#include <cstring>
#include <memory>
#include <utility>

#include "allocate.hpp"
#include "item_ref.hpp"
#include "bit_vector.hpp"
#include "static_vector.hpp"
#include <tdc/math/imath.hpp>

namespace tdc {
namespace vec {

/// \brief A vector of integers of fixed bit width, using bit packing to minimize the required space. Please use the \ref FixedWidthIntVector alias.
///
/// This is an alternative to \ref IntVector for when the integer width is known at compile-time.
/// Thus, it is faster than \ref IntVector, but it still uses bit masking operations.
/// Hence, for bit widths that can be represented by primitive types (8, 16, 32), it is recommended to use primitive alternatives.
/// The \ref FixedWidthIntVector alias will automatically select the best variant and is therefore recommended over using this class directly.
///
/// Int vectors are static, i.e., integers cannot be inserted or deleted.
///
/// \tparam m_width the bit width of stored integers, at most 64
template<size_t m_width>
class FixedWidthIntVector_ {
private:
    static_assert(m_width <= 64ULL, "Maximum width of 63 bits exceeded.");
    static_assert(m_width != 8 && m_width != 16 && m_width != 32 && m_width != 64,
        "Integers of the selected bit width can be represented by primitive types. You really don't want to use this class for this case, please use the FixedWidthIntVector alias instead.");

    friend class ItemRef<FixedWidthIntVector_<m_width>, uint64_t>;

    inline static constexpr uint64_t bit_mask(const uint64_t bits) {
        return (1ULL << bits) - 1ULL;
    }
    
    static constexpr uint64_t m_mask = bit_mask(m_width);

    size_t m_size;
    std::unique_ptr<uint64_t[]> m_data;

    uint64_t get(const size_t i) const {
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
    
    void set(const size_t i, const uint64_t v_) {
        const uint64_t v = v_ & m_mask; // make sure it fits...
        
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

            if(dvl == 64ULL) {        
                m_data[a] = lo | (v << dl);
            } else {
                const uint64_t hi = xa >> dvl;
                m_data[a] = lo | (v << dl) | (hi << dvl);
            }
        }
    }

public:
    /// \brief Proxy for reading and writing a single integer.
    using IntRef = ItemRef<FixedWidthIntVector_<m_width>, uint64_t>;

    /// \brief Constructs an empty integer vector of zero length.
    inline FixedWidthIntVector_() : m_size(0) {
    }

    /// \brief Copy constructor.
    /// \param other the integer vector to copy
    inline FixedWidthIntVector_(const FixedWidthIntVector_<m_width>& other) {
        *this = other;
    }

    /// \brief Move constructor.
    /// \param other the integer vector to move
    inline FixedWidthIntVector_(FixedWidthIntVector_<m_width>&& other) {
        *this = std::move(other);
    }

    /// \brief Constructs an integer vector with the specified length.
    /// \param size the number of integers
    /// \param initialize if \c true, the integers will be initialized with zero
    inline FixedWidthIntVector_(const size_t size, const bool initialize = true) {
        m_size = size;
        m_data = allocate_integers(size, m_width, initialize);
    }

    /// \brief Copy assignment.
    /// \param other the integer vector to copy
    inline FixedWidthIntVector_& operator=(const FixedWidthIntVector_<m_width>& other) {
        m_size = other.m_size;
        m_data = allocate_integers(m_size, m_width, false);
        memcpy(m_data.get(), other.m_data.get(), math::idiv_ceil(m_size * m_width, 64ULL));
        return *this;
    }

    /// \brief Move assignment.
    /// \param other the integer vector to move
    inline FixedWidthIntVector_& operator=(FixedWidthIntVector_&& other) {
        m_size = other.m_size;
        m_data = std::move(other.m_data);
        return *this;
    }

    /// \brief Resizes the integer vector with the specified new length and current bit width.
    ///
    /// \param size the new number of integers
    inline void resize(const size_t size) {
        FixedWidthIntVector_<m_width> new_iv(size, size >= m_size); // no init needed if new size is smaller
        
        const size_t num_to_copy = std::min(size, m_size);
        for(size_t i = 0; i < num_to_copy; i++) {
            new_iv.set(i, get(i));
        }
        
        *this = std::move(new_iv);
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

    /// \brief The width of each integer in bits.
    static constexpr size_t width() {
        return m_width;
    }
    
    /// \brief The number of integers in the vector.
    inline size_t size() const {
        return m_size;
    }
};

/// \cond INTERNAL
template<size_t m_width> struct FixedWidthIntVector_Selector { using type = FixedWidthIntVector_<m_width>; };
template<> struct FixedWidthIntVector_Selector<1> { using type = BitVector; };
template<> struct FixedWidthIntVector_Selector<8> { using type = StaticVector<uint8_t>; };
template<> struct FixedWidthIntVector_Selector<16> { using type = StaticVector<uint16_t>; };
template<> struct FixedWidthIntVector_Selector<32> { using type = StaticVector<uint32_t>; };
template<> struct FixedWidthIntVector_Selector<64> { using type = StaticVector<uint64_t>; };
/// \endcond INTERNAL

/// \brief Alias for vectors of fixed width integers.
///
/// Generally, this delegates to \ref FixedWidthIntVector_ with the given width.
/// However, for widths that are multiples of eight, the corresponding version of \ref StaticVector will be chosen for performance reasons.
/// Furthermore, should the width equal one, \ref BitVector will selected.
///
/// \tparam m_width the bit width of stored integers
template<size_t m_width> using FixedWidthIntVector = typename FixedWidthIntVector_Selector<m_width>::type;

}} // namespace tdc::vec
