#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <vector>

#include "factor.hpp"

namespace tdc {
namespace comp {
namespace lz77 {

class FactorBuffer {
private:
    std::vector<Factor> factors_;

public:
    struct Stats {
        size_t input_size;
        
        size_t num_refs;
        size_t num_literals;
        
        size_t min_ref_len;
        size_t max_ref_len;
        double avg_ref_len;
        
        size_t min_ref_dist;
        size_t max_ref_dist;
        double avg_ref_dist;

        Stats() : input_size(0),
                  num_refs(0),
                  num_literals(0),
                  min_ref_len(SIZE_MAX),
                  max_ref_len(0),
                  avg_ref_len(0.0),
                  min_ref_dist(SIZE_MAX),
                  max_ref_dist(0),
                  avg_ref_dist(0.0) {
        }
    };

    inline FactorBuffer() {
    }

    void emplace_back(const char_t literal) {
        factors_.emplace_back(literal);
    }

    void emplace_back(const index_t src, const index_t len) {
        factors_.emplace_back(src, len);
    }

    const Factor* factors() const {
        return factors_.data();
    }

    size_t size() const {
        return factors_.size();
    }

    Stats gather_stats() const {
        Stats stats;
        size_t i = 0;
        size_t total_ref_len = 0;
        size_t total_ref_dist = 0;
        
        for(const auto& f : factors_) {
            if(f.is_reference()) {
                ++stats.num_refs;
                
                stats.min_ref_len = std::min(stats.min_ref_len, (size_t)f.len);
                stats.max_ref_len = std::max(stats.max_ref_len, (size_t)f.len);
                total_ref_len += f.len;

                assert(i > f.src);
                const size_t dist = i - f.src;
                stats.min_ref_dist = std::min(stats.min_ref_dist, dist);
                stats.max_ref_dist = std::max(stats.max_ref_dist, dist);
                total_ref_dist += dist;
                
                i += f.len;
            } else {
                ++stats.num_literals;
                ++i;
            }
        }

        if(stats.num_refs > 0) {
            stats.avg_ref_len  = (double)total_ref_len  / (double)stats.num_refs;
            stats.avg_ref_dist = (double)total_ref_dist / (double)stats.num_refs;
        } else {
            stats.min_ref_len  = 0;
            stats.min_ref_dist = 0;
        }
        
        stats.input_size = i;
        return stats;
    }
};

}}}
