#pragma once

#include <memory>
#include <utility>

#include <tdc/vec/bit_vector.hpp>
#include <tdc/vec/bit_rank.hpp>
#include <tdc/vec/bit_select.hpp>
#include <tdc/util/assert.hpp>

namespace tdc {
namespace vec {

/// \brief Space efficient representation of a sorted sequence using gap encoding.
///
/// Items in the sequence are stored as their difference from the respective previous item in unary encoding,
/// requiring <tt>D+n</tt> bits with \c D the difference between minimum and maximum and \c n the number of items in the sequence.
/// Constant-time access is provided via binary rank and select queries, which require some additional space.
class SortedSequence {
private:
    uint64_t                   m_first;
    size_t                     m_size;
    std::shared_ptr<BitVector> m_bits;
    BitRank<>                  m_rank;
    BitSelect0                 m_sel0;

    template<typename T>
    size_t encode_unary(size_t pos, T value) {
        auto& bits = *m_bits;
        while(value) {
            bits[pos++] = 1;
            --value;
        }
        bits[pos++] = 0;
        return pos;
    }

public:
    /// \brief Construct an empty sequence.
    inline SortedSequence() : m_size(0) {
    }

    /// \brief Constructs a compressed sequence from the given array.
    /// \tparam the array type, must support the <tt>[]</tt> operator and items must be convertible to unsigned 64-bit integers
    /// \param array the array, items must be in ascending order
    /// \param size the number of items in the array
    template<typename array_t>
    SortedSequence(const array_t& array, const size_t size) : m_size(size) {
        assert_sorted_ascending(array, size);

        if(m_size > 0) {
            m_first = array[0];
            {
                const size_t max = size_t(array[m_size-1]);
                const size_t num_bits = m_size + (max - m_first);
                m_bits = std::make_shared<BitVector>(num_bits);

                auto prev = m_first;
                size_t pos = encode_unary(0, 0);
                for(size_t i = 1; i < m_size; i++) {
                    const auto v = array[i];
                    pos = encode_unary(pos, v - prev);
                    prev = v;
                }

                assert(pos == num_bits);
            }
            
            // construct rank1 + select0
            m_rank = BitRank(m_bits);
            m_sel0 = BitSelect0(m_bits);
        }
    }

    /// \brief Copy constructor.
    /// \param other the object to copy
    inline SortedSequence(const SortedSequence& other) {
        *this = other;
    }

    /// \brief Move constructor.
    /// \param other the object to move
    inline SortedSequence(SortedSequence&& other) {
        *this = std::move(other);
    }

    /// \brief Copy assignment.
    /// \param other the object to copy
    inline SortedSequence& operator=(const SortedSequence& other) {
        m_size = other.m_size;
        m_first = other.m_first;
        m_bits = other.m_bits;
        m_rank = other.m_rank;
        m_sel0 = other.m_sel0;
        return *this;
    }

    /// \brief Move assignment.
    /// \param other the object to move
    inline SortedSequence& operator=(SortedSequence&& other) {
        m_size = other.m_size;
        m_first = other.m_first;
        m_bits = std::move(other.m_bits);
        m_rank = std::move(other.m_rank);
        m_sel0 = std::move(other.m_sel0);
        return *this;
    }

    /// \brief Returns an element from the sequence.
    /// \param i the index of the element to return
    inline uint64_t operator[](size_t i) const {
        assert(i < m_size);
        return m_first + m_rank(m_sel0(i+1));
    }

    /// \brief Returns the number of elements in the sequence.
    inline size_t size() const {
        return m_size;
    }
};

}} // namespace tdc::vec
