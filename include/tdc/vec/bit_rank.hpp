#pragma once

#include <memory>
#include <utility>

#include "bit_vector.hpp"
#include "int_vector.hpp"

namespace tdc {
namespace vec {

/// \brief A space efficient data structure for answering rank queries on a \ref BitVector in constant time.
///
/// A rank query counts the number of set or unset bits, respectively, from the beginning up to a given position.
/// The data structure uses a hierarchical scheme dividing the bit vector into \em blocks (of 64 bits) and
/// \em superblocks (of 4096 bits) and precomputes a number of rank queries, storing them in a space efficient manner.
/// On the lowest level, it uses \c popcnt instructions.
///
/// Note that this data structure is \em static.
/// It maintains a pointer to the underlying bit vector and will become invalid if that bit vector is changed after construction.
class BitRank {
private:
    static constexpr size_t SUP_SZ = 4096;
    static constexpr size_t SUP_MSK = 4095;
    static constexpr size_t SUP_W = 12;

    static constexpr size_t BLOCKS_PER_SB = SUP_SZ >> 6ULL;
    static constexpr size_t SB_INNER_RS = SUP_W - 6ULL;
    
    std::shared_ptr<const BitVector> m_bv;

    IntVector m_blocks;    // size 64 each
    IntVector m_supblocks; // size SUP_SZ each

public:
    /// \brief Constructs the rank data structure for the given bit vector.
    /// \param bv the bit vector
    BitRank(std::shared_ptr<const BitVector> bv);

    /// \brief Constructs an empty, uninitialized rank data structure.
    inline BitRank() {
    }

    /// \brief Copy constructor.
    /// \param other the object to copy
    inline BitRank(const BitRank& other) {
        *this = other;
    }

    /// \brief Move constructor.
    /// \param other the object to move
    inline BitRank(BitRank&& other) {
        *this = std::move(other);
    }

    /// \brief Copy assigment.
    /// \param other the object to copy
    inline BitRank& operator=(const BitRank& other) {
        m_bv = other.m_bv;
        m_blocks = other.m_blocks;
        m_supblocks = other.m_supblocks;
        return *this;
    }

    /// \brief Move assigment.
    /// \param other the object to move
    inline BitRank& operator=(BitRank&& other) {
        m_bv = std::move(other.m_bv);
        m_blocks = std::move(other.m_blocks);
        m_supblocks = std::move(other.m_supblocks);
        return *this;
    }

    /// \brief Counts the number of set bit (1-bits) from the beginning of the bit vector up to (and including) position \c x.
    /// \param x the position until which to count
    size_t rank1(const size_t x) const;

    /// \brief Counts the number of set bits from the beginning of the bit vector up to (and including) position \c x.
    ///
    /// This is a convenience alias for \ref rank1.
    ///
    /// \param x the position until which to count
    inline size_t operator()(size_t x) const {
        return rank1(x);
    }

    /// \brief Counts the number of unset bits (0-bits) from the beginning of the bit vector up to (and including) position \c x.
    /// \param x the position until which to count
    inline size_t rank0(size_t x) const {
        return x + 1 - rank1(x);
    }
};

}} // namespace tdc::vec
