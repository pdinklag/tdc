#pragma once

#include <cstdint>
#include <cstring>
#include <memory>
#include <vector>
#include <utility>

#include "util.hpp"

namespace tdc {
namespace vec {

/// \brief A vector of bits, using bit packing to minimize the required space.
///
/// Bit vectors are static, i.e., bits cannot be inserted or deleted.
class BitVector {
private:
    inline static constexpr size_t block(size_t i) {
        return i >> 6ULL; // divide by 64
    }

    inline static constexpr size_t offset(size_t i) {
        return i & 63ULL; // mod 64
    }

    inline static std::unique_ptr<uint64_t[]> allocate(size_t num_bits) {
        const size_t num64 = div64_ceil(num_bits);
        uint64_t* p = new uint64_t[num64];
        memset(p, 0, num64 * sizeof(uint64_t));
        return std::unique_ptr<uint64_t[]>(p);
    }

    size_t m_size;
    std::unique_ptr<uint64_t[]> m_bits;

    inline bool bitread(size_t i) const {
        //~ const size_t q = block(i);
        //~ const size_t k = offset(i);
        //~ const uint64_t mask = (1ULL << k);
        //~ return bool(m_bits[q] & mask);
        
        // more optimization-friendly:
        return bool(m_bits[i >> 6ULL] & (1ULL << (i & 63ULL)));
    }

    inline void bitset(size_t i, bool b) {
        const size_t q = block(i);
        const size_t mask = (1ULL << offset(i));
        m_bits[q] = (m_bits[q] & ~mask) | (-b & mask);
    }

public:
    /// \brief Proxy for reading and writing a single bit.
    struct BitRef {
        /// \brief The bit vector this proxy belongs to.
        BitVector* bv;
        
        /// \brief The number of the referred bit.
        size_t i;

        /// \brief Reads the referred bit.
        inline operator bool() const {
            return bv->bitread(i);
        }

        /// \brief Writes the referred bit.
        /// \param b the bit to write
        inline void operator=(bool b) {
            bv->bitset(i, b);
        }
    };

    /// \brief Constructs an empty bit vector of zero length.
    inline BitVector() : m_size(0) {
    }

    /// \brief Copy constructor.
    /// \param other the bit vector to copy.
    inline BitVector(const BitVector& other) {
        *this = other;
    }

    /// \brief Move constructor.
    /// \param other the bit vector to move.
    inline BitVector(BitVector&& other) {
        *this = std::move(other);
    }

    /// \brief Constructs a bit vector of the specified length with all bits initialized to zero.
    /// \param size the number of bits in the bit vector
    inline BitVector(size_t size) : m_size(size) {
        m_bits = allocate(m_size);
    }

    /// \brief Copy assignment.
    /// \param other the bit vector to copy.
    inline BitVector& operator=(const BitVector& other) {
        m_size = other.m_size;
        m_bits = allocate(m_size);
        std::memcpy(m_bits.get(), other.m_bits.get(), div64_ceil(m_size));
        return *this;
    }

    /// \brief Move assignment.
    /// \param other the bit vector to move.
    inline BitVector& operator=(BitVector&& other) {
        m_size = std::move(other.m_size);
        m_bits = std::move(other.m_bits);
        return *this;
    }

    /// \brief Gets all bits in the interval <tt>[64i,64i+63]</tt> packed in a 64-bit integer, with the least significant bit representing bit <tt>64i</tt>.
    /// \param i the block index.
    inline uint64_t block64(size_t i) const {
        return m_bits[i];
    }

    /// \brief The number of 64-bit blocks contained in this bit vector.
    inline size_t num_blocks() const {
        return div64_ceil(m_size);
    }

    /// \brief Reads the specified bit.
    /// \param i the number of the bit to read.
    inline bool operator[](size_t i) const {
        return bitread(i);
    }

    /// \brief Access to the specified bit.
    /// \param i the number of the bit to access.
    inline BitRef operator[](size_t i) {
        return BitRef { this, i };
    }

    /// \brief The number of bits contained in this bit vector.
    inline size_t size() const {
        return m_size;
    }
};

}} // namespace tdc::vec
