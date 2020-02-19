#include <tdc/vec/bit_rank.hpp>
#include <tdc/vec/rank_u64.hpp>
#include <tdc/vec/util.hpp>

using namespace tdc::vec;

BitRank::BitRank(std::shared_ptr<const BitVector> bv) : m_bv(bv) {
    const size_t n = m_bv->size();

    // determine number of superblocks and superblock entry width
    const size_t sw = log2_ceil(n-1);
    const size_t sq = n >> SUP_W;
    const size_t sm = n & SUP_MSK;
    m_supblocks.resize(sm ? sq + 1 : sq, sw);

    // determine number of blocks and block width
    const size_t bq = n >> 6ULL; // div 64
    const size_t bm = n & 63ULL; // mod 64
    m_blocks.resize(bm ? bq + 1 : bq, SUP_W);

    // construct
    {
        size_t rank_bv = 0; // 1-bits in whole BV
        size_t rank_sb = 0; // 1-bits in current superblock
        size_t cur_sb = 0;  // current superblock

        for(size_t j = 0; j < m_blocks.size(); j++) {
            size_t i = j >> SB_INNER_RS;
            if(i > cur_sb) {
                // we reached a new superblock
                m_supblocks[cur_sb] = rank_bv;
                rank_sb = 0;
                cur_sb = i;
            }

            auto rank_b = rank1_u64(m_bv->block64(j));
            rank_sb += rank_b;
            rank_bv += rank_b;

            m_blocks[j] = rank_sb;
        }
    }
}

size_t BitRank::rank1(const size_t x) const {
    size_t r = 0;
    const size_t i = x >> SUP_W;
    if(i > 0) r += m_supblocks[i - 1];
    const size_t j = x >> 6ULL;

    const size_t k = j - i * BLOCKS_PER_SB;
    if(k > 0) r += m_blocks[j - 1];

    r += rank1_u64(m_bv->block64(j), x & 63ULL);
    return r;
}
