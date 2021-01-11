#pragma once
    
#include <algorithm>
#include <cstring>
#include <memory>
#include <utility>

#include "allocate.hpp"
#include "item_ref.hpp"
#include "iterator.hpp"
#include "bit_vector.hpp"
#include "static_vector.hpp"
#include "vector_builder.hpp"

#include <tdc/math/idiv.hpp>
#include <tdc/math/bit_mask.hpp>
#include <tdc/uint/uint40.hpp>

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
    static_assert(m_width != 8 && m_width != 16 && m_width != 32 && m_width != 40 && m_width != 64,
        "Integers of the selected bit width can be represented by primitive types. You really don't want to use this class for this case, please use the FixedWidthIntVector alias instead.");

public:
    /// \brief The \ref VectorBuilder type for fixed integer vectors.
    using builder_type = VectorBuilder<FixedWidthIntVector_<m_width>>;
    
private:
    friend class ItemRef<FixedWidthIntVector_<m_width>, uint64_t>;
    friend class ConstItemRef<FixedWidthIntVector_<m_width>, uint64_t>;

    static constexpr uint64_t m_mask = math::bit_mask<uint64_t>(m_width);

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
            const uint64_t a_lo = m_data[a] & math::bit_mask<uint64_t>(da);
            const uint64_t v_lo = v & math::bit_mask<uint64_t>(wa);
            m_data[a] = (v_lo << da) | a_lo;

            // combine the db highest bits of b and the wb highest bits of v
            const uint64_t b_hi = m_data[b] >> wb;
            const uint64_t v_hi = v >> wa;
            m_data[b] = (b_hi << wb) | v_hi;
        } else {
            const size_t dl = j & 63ULL;
            const uint64_t xa = m_data[a];
            const uint64_t mask_lo = math::bit_mask<uint64_t>(dl);
            const uint64_t mask_hi = ~mask_lo << m_width;
            
            m_data[a] = (xa & mask_lo) | (v << dl) | (xa & mask_hi);
        }
    }

public:
    /// \brief Proxy for reading and writing a single integer.
    using IntRef = ItemRef<FixedWidthIntVector_<m_width>, uint64_t>;
    
    /// \brief Proxy for reading a single integer.
    using ConstIntRef = ConstItemRef<FixedWidthIntVector_<m_width>, uint64_t>;

    /// \brief Constructs an empty integer vector of zero length.
    inline FixedWidthIntVector_() : m_size(0) {
    }

    /// \brief Constructs an integer vector with the specified length.
    /// \param size the number of integers
    /// \param initialize if \c true, the integers will be initialized with zero
    inline FixedWidthIntVector_(const size_t size, const bool initialize = true) {
        m_size = size;
        m_data = allocate_integers(size, m_width, initialize);
    }

    inline FixedWidthIntVector_(const FixedWidthIntVector_& other) { *this = other; }
    FixedWidthIntVector_(FixedWidthIntVector_<m_width>&& other) = default;

    inline FixedWidthIntVector_& operator=(const FixedWidthIntVector_& other) {
        m_size = other.m_size;
        m_data = allocate_integers(m_size, m_width, false);
        memcpy(m_data.get(), other.m_data.get(), math::idiv_ceil(m_size * m_width, 64ULL) * sizeof(uint64_t));
        return *this;
    }

    FixedWidthIntVector_& operator=(FixedWidthIntVector_&& other) = default;

    /// \brief Exchanges the contents of this vector with that of the other.
    ///
    /// \param other the other vector
    inline void swap(FixedWidthIntVector_& other) {
        std::swap(m_size, other.m_size);
        std::swap(m_data, other.m_data);
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
    static constexpr size_t width() {
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

/// \cond INTERNAL
template<size_t m_width> struct FixedWidthIntVector_Selector { using type = FixedWidthIntVector_<m_width>; };
template<> struct FixedWidthIntVector_Selector<1> { using type = BitVector; };
template<> struct FixedWidthIntVector_Selector<8> { using type = StaticVector<uint8_t>; };
template<> struct FixedWidthIntVector_Selector<16> { using type = StaticVector<uint16_t>; };
template<> struct FixedWidthIntVector_Selector<32> { using type = StaticVector<uint32_t>; };
template<> struct FixedWidthIntVector_Selector<40> { using type = StaticVector<uint40_t>; };
template<> struct FixedWidthIntVector_Selector<64> { using type = StaticVector<uint64_t>; };
/// \endcond INTERNAL

/// \brief Alias for vectors of fixed width integers.
///
/// Generally, this delegates to \ref FixedWidthIntVector_ with the given width.
/// However, for widths that are multiples of eight, the corresponding version of \ref StaticVector will be chosen for performance reasons.
/// Furthermore, should the width equal one, \ref BitVector will selected.
///
/// \tparam m_width the bit width of stored integers
template<size_t m_width>
class FixedWidthIntVector : public FixedWidthIntVector_Selector<m_width>::type {
private:
    using base_t = typename FixedWidthIntVector_Selector<m_width>::type;

public:
    /// \brief The \ref tdc::vec::VectorBuilder type for fixed width integer vectors.
    using builder_type = typename base_t::builder_type;

    using base_t::base_t;
};

}} // namespace tdc::vec
