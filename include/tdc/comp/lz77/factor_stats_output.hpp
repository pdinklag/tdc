#pragma once

#include <algorithm>
#include <cstddef>

#include "factor.hpp"

namespace tdc {
namespace comp {
namespace lz77 {

class FactorStatsOutput {
public:
    struct Stats {
        size_t input_size;
        
        size_t num_refs;
        size_t num_literals;
        
        size_t min_ref_len;
        size_t max_ref_len;
        size_t total_ref_len;
        
        size_t min_ref_dist;
        size_t max_ref_dist;
        size_t total_ref_dist;

        Stats() : input_size(0),
                  num_refs(0),
                  num_literals(0),
                  min_ref_len(SIZE_MAX),
                  max_ref_len(0),
                  total_ref_len(0),
                  min_ref_dist(SIZE_MAX),
                  max_ref_dist(0),
                  total_ref_dist(0) {
        }
    };

protected:
    Stats stats_;

    void update_stats(Factor&& f) {
        if(f.is_reference()) {
            ++stats_.num_refs;

            assert(f.src < stats_.input_size);
            const size_t dist = stats_.input_size - f.src;
            stats_.min_ref_dist = std::min(stats_.min_ref_dist, dist);
            stats_.max_ref_dist = std::max(stats_.max_ref_dist, dist);
            stats_.total_ref_dist += dist;

            stats_.min_ref_len = std::min(stats_.min_ref_len, (size_t)f.len);
            stats_.max_ref_len = std::max(stats_.max_ref_len, (size_t)f.len);
            stats_.total_ref_len  += f.len;
        } else {
            ++stats_.num_literals;
        }
        stats_.input_size += f.decoded_length();
    }

public:
    inline FactorStatsOutput() {
    }

    FactorStatsOutput(const FactorStatsOutput&) = default;
    FactorStatsOutput(FactorStatsOutput&&) = default;
    FactorStatsOutput& operator=(const FactorStatsOutput&) = default;
    FactorStatsOutput& operator=(FactorStatsOutput&&) = default;

    void emplace_back(const char_t literal) {
        update_stats(Factor(literal));
    }

    void emplace_back(const index_t src, const index_t len) {
        update_stats(Factor(src, len));
    }

    void emplace_back(Factor&& f) {
        update_stats(std::move(f));
    }

    void emplace_back(const Factor& f) {
        update_stats(Factor(f));
    }
    
    size_t size() const {
        return stats_.num_literals + stats_.num_refs;
    }

    const Stats& stats() const {
        return stats_;
    }
};

}}}
