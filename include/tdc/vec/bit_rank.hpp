#pragma once

#include <memory>
#include <utility>

#include <tdc/math/idiv.hpp>
#include <tdc/util/rank_u64.hpp>

#include "bit_vector.hpp"
#include "fixed_width_int_vector.hpp"

namespace tdc {
namespace vec {

/// \brief A space efficient data structure for answering rank queries on a \ref BitVector in constant time.
///
/// A rank query counts the number of set or unset bits, respectively, from the beginning up to a given position.
/// The data structure uses a hierarchical scheme dividing the bit vector into \em blocks (of 64 bits) and
/// \em superblocks and precomputes a number of rank queries, storing them in a space efficient manner.
/// On the lowest level, it uses \c popcnt instructions.
///
/// The size of a superblock is configurable via the template parameter.
/// The default value of 12 yields a very good trade-off between time and space.
/// However, query times can be reduced by about 15% by setting it to \c 16, which, however, will increase space usage by about 25%.
///
/// Note that this data structure is \em static.
/// It maintains a pointer to the underlying bit vector and will become invalid if that bit vector is changed after construction.
///
/// \tparam t_supblock_bit_width the bit width of superblock entries, a superblock will contain <tt>2^t_supblock_bit_width</tt> bits
template<uint64_t t_supblock_bit_width = 12>
class BitRank {
private:
    static constexpr size_t SUP_W = t_supblock_bit_width;
    static constexpr size_t SUP_SZ = 1ULL << SUP_W;
    static constexpr size_t BLOCKS_PER_SB = SUP_SZ >> 6ULL;
    
    std::shared_ptr<const BitVector> m_bv;

    FixedWidthIntVector<SUP_W> m_blocks; // size 64 each
    FixedWidthIntVector<64> m_supblocks; // size SUP_SZ each

public:
    /// \brief Constructs the rank data structure for the given bit vector.
    /// \param bv the bit vector
    BitRank(std::shared_ptr<const BitVector> bv) : m_bv(bv) {
        const size_t n = m_bv->size();

        m_blocks = FixedWidthIntVector<SUP_W>(math::idiv_ceil(n, 64ULL), false);
        m_supblocks = FixedWidthIntVector<64>(math::idiv_ceil(n, SUP_SZ), false);    

        // construct
        {
            size_t rank_bv = 0; // 1-bits in whole BV
            size_t rank_sb = 0; // 1-bits in current superblock
            size_t cur_sb = 0;  // current superblock

            for(size_t j = 0; j < m_blocks.size(); j++) {
                if(j % BLOCKS_PER_SB == 0) {
                    // we reached a new superblock
                    m_supblocks[cur_sb++] = rank_bv;
                    assert(m_supblocks[cur_sb-1] == rank_bv);
                    rank_sb = 0;
                }
                
                m_blocks[j] = rank_sb;

                const auto rank_b = rank1_u64(m_bv->block64(j));
                rank_sb += rank_b;
                rank_bv += rank_b;
            }
        }
    }

    /// \brief Constructs an empty, uninitialized rank data structure.
    inline BitRank() {
    }

    BitRank(const BitRank& other) = default;
    BitRank(BitRank&& other) = default;
    BitRank& operator=(const BitRank& other) = default;
    BitRank& operator=(BitRank&& other) = default;

    /// \brief Counts the number of set bit (1-bits) from the beginning of the bit vector up to (and including) position \c x.
    /// \param x the position until which to count
    size_t rank1(const size_t x) const {
        const size_t r_sb = m_supblocks[x / SUP_SZ];
        const size_t j   = x >> 6ULL;
        const size_t r_b = m_blocks[j];
        
        return r_sb + r_b + rank1_u64(m_bv->block64(j), x & 63ULL);
    }

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
