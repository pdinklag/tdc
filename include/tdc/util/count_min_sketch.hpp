#pragma once

#include <cstdint>
#include <cstddef>
#include <limits>
#include <random>

#include <tdc/math/bit_mask.hpp>
#include <tdc/math/ilog2.hpp>
#include <tdc/random/seed.hpp>
#include <tdc/uint/uint_half.hpp>
#include <tdc/util/index.hpp>

namespace tdc {

/// \brief Generic count-min sketch.
/// \tparam Key key type
template<typename Key>
class CountMinSketch {
private:
    using HalfKey = uint_half_t<Key>;

    index_t width_mask_;
    index_t height_;

    std::vector<HalfKey>              hash_mul_;
    std::vector<std::vector<index_t>> count_;

    index_t hash(const size_t j, const Key& key) const {
        constexpr size_t KEY_HALF_BITS = std::numeric_limits<Key>::digits / 2;
        index_t h = ((HalfKey)key * hash_mul_[j]) ^ ((HalfKey)(key >> KEY_HALF_BITS) * hash_mul_[j]);

        // modulo 2^19 - 1 (Mersenne prime)
        {
            constexpr index_t MERSENNE19 = (1U << 19) - 1;
            const auto v = h + 1;
            const auto z = ((v >> 19) + v) >> 19;
            h = (h + z) & MERSENNE19;
        }
        
        return h & width_mask_;
    }

public:
    /// \brief Constructs an empty sketch.
    /// \param width the width of the sketch (will be rounded to the next power of two)
    /// \param height the height of the sketch
    CountMinSketch(const size_t width, const size_t height) : height_(height) {
        const auto wbits = math::ilog2_ceil(width - 1);
        width_mask_ = math::bit_mask<index_t>(wbits);

        count_.reserve(height_);
        for(size_t j = 0; j < height_; j++) {
            count_.emplace_back(1ULL << wbits, index_t(0));
        }
        
        hash_mul_.reserve(height);

        {
            /*
             * generate random multipliers such that all nibbles are non-zero
             */
            static constexpr size_t num_nibbles = (std::numeric_limits<HalfKey>::digits / 4);
            
            std::default_random_engine gen(random::DEFAULT_SEED);
            std::uniform_int_distribution<HalfKey> random_nibble(0x1, 0xF); // random
            
            for(size_t j = 0; j < height_; j++) {
                HalfKey mul = 0;
                for(size_t i = 0; i < num_nibbles; i++) {
                    mul = (mul << 4) | random_nibble(gen);
                }
                hash_mul_.push_back(mul);
            }
        }
    }

    /// \brief Counts the specified key in the sketch.
    /// \param key the key to count
    /// \param times the amount of times to count the key
    void count(const Key& key, index_t times) {
        for(size_t j = 0; j < height_; j++) {
            count_[j][hash(j, key)] += times;
        }
    }

    /// \brief Counts the specified key in the sketch and computes a new estimate.
    /// \param key the key to count
    /// \param times the amount of times to count the key
    /// \return the estimated frequency of the key in the sketch
    index_t count_and_estimate(const Key& key, index_t times) {
        index_t est = INDEX_MAX;
        for(size_t j = 0; j < height_; j++) {
            auto& row = count_[j];
            const auto h = hash(j, key);
            row[h] += times;
            est = std::min(est, row[h]);
        }
        return est;
    }
};

} // namespace tdc
