#pragma once

#include <algorithm>
#include <cassert>
#include <vector>

#include "factor_stats_output.hpp"

namespace tdc {
namespace comp {
namespace lz77 {

class FactorBuffer : public FactorStatsOutput {
public:
    template<typename FactorOutput>
    static void merge(const FactorBuffer& a, const FactorBuffer& b, FactorOutput& out) {
        auto a_next = a.factors_.begin();
        auto a_end  = a.factors_.end();
        auto b_next = b.factors_.begin();
        auto b_end  = b.factors_.end();

        Factor a_cur = a_next != a_end ? *a_next++ : Factor();
        size_t a_pos = 0;
        
        Factor b_cur = b_next != b_end ? *b_next++ : Factor();
        size_t b_pos = 0;
        
        size_t i = 0;
        
        while(a_cur.is_valid() && b_cur.is_valid()) {
            // invariant: a_pos == b_pos == i
            assert(a_pos == i);
            assert(b_pos == i);

            // greedily select next factor
            Factor f;
            {
                const auto a_len = a_cur.decoded_length();
                const auto b_len = b_cur.decoded_length();

                // pick longer factor, or the rightmost if both are equally long
                const bool pick_a = (a_len > b_len) || (a_len == b_len && a_cur.src >= b_cur.src);
                f = pick_a ? a_cur : b_cur;

                // try and avoid references of length 1
                if(f.is_reference() && f.len == 1) {
                    f = pick_a ? b_cur : a_cur; // the other one must be a literal
                    assert(f.is_literal());
                }
            }

            // emit factor
            i += f.decoded_length();
            out.emplace_back(f);

            // advance
            static auto advance = [](Factor& cur, auto& next, const auto& end){
                if(cur.len <= 1) {
                    // reference of length 1 or literal, advance to next
                    if(next != end) {
                        cur = *next++;
                    } else {
                        cur = Factor();
                    }
                } else {
                    // reference of length >1, increment position and reduce length
                    --cur.len;
                    ++cur.src;
                }
            };

            // advance A
            for(; a_pos < i && a_cur.is_valid(); a_pos++) {
                advance(a_cur, a_next, a_end);
            }

            // advance B
            for(; b_pos < i && b_cur.is_valid(); b_pos++) {
                advance(b_cur, b_next, b_end);
            }
        }

        // handle remainder (if any)
        if(a_cur.is_valid()) assert(!b_cur.is_valid()); // either one can be valid
        if(b_cur.is_valid()) assert(!a_cur.is_valid()); // not both
        
        while(a_cur.is_valid() && a_next != a_end) {
            out.emplace_back(a_cur);
            a_cur = *a_next++;
        }
        while(b_cur.is_valid() && b_next != b_end) {
            out.emplace_back(b_cur);
            b_cur = *b_next++;
        }
    }

private:
    std::vector<Factor> factors_;
    bool track_stats_;

public:
    inline FactorBuffer(bool track_stats = false) : track_stats_(track_stats) {
    }

    FactorBuffer(const FactorBuffer&) = default;
    FactorBuffer(FactorBuffer&&) = default;
    FactorBuffer& operator=(const FactorBuffer&) = default;
    FactorBuffer& operator=(FactorBuffer&&) = default;

    void emplace_back(const char_t literal) {
        if(track_stats_) FactorStatsOutput::emplace_back(literal);
        factors_.emplace_back(literal);
    }

    void emplace_back(const index_t src, const index_t len) {
        if(track_stats_) FactorStatsOutput::emplace_back(src, len);
        factors_.emplace_back(src, len);
    }

    void emplace_back(Factor&& f) {
        if(track_stats_) FactorStatsOutput::emplace_back(f.src, f.len);
        factors_.emplace_back(std::move(f));
    }

    void emplace_back(const Factor& f) {
        if(track_stats_) FactorStatsOutput::emplace_back(f);
        factors_.emplace_back(f);
    }

    const Factor* factors() const {
        return factors_.data();
    }

    size_t size() const {
        assert(!track_stats_ || FactorStatsOutput::size() == factors_.size());
        return factors_.size();
    }
};

}}}
