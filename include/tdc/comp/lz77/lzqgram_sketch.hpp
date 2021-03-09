#pragma once

#include <bit>
#include <cstdint>
#include <concepts>
#include <iostream>
#include <limits>
#include <stdexcept>

#include <tdc/math/prime.hpp>
#include <tdc/sketch/count_min.hpp>
#include <tdc/util/index.hpp>
#include <tdc/util/min_count.hpp>

#include <robin_hood.h>

#include "stats.hpp"

namespace tdc {
namespace comp {
namespace lz77 {
namespace qgram {

template<size_t m_filter_size, size_t m_sketch_cols, size_t m_sketch_rows, std::unsigned_integral qgram_t = uint64_t>
class SketchProcessor {
private:
    static constexpr uint64_t m_filter_prime = math::prime_predecessor(m_filter_size);

    class ModuloHash {
    public:
        size_t operator()(const qgram_t key) const {
            return (size_t)(key % m_filter_prime);
        }
    };
    
    struct FilterEntry {
        qgram_t pattern;
        index_t seen_at;
        
        index_t old_count;
        index_t new_count;
        
        MinCount<FilterEntry*>::Entry min_ds_entry;
        
        FilterEntry() : FilterEntry(0, 0, 0) {
        }
        
        FilterEntry(qgram_t _pattern, index_t _seen_at, index_t _count) : pattern(_pattern), seen_at(_seen_at), old_count(_count), new_count(_count) {
        }
        
        FilterEntry(FilterEntry&&) = default;
        FilterEntry(const FilterEntry&) = default;
        FilterEntry& operator=(FilterEntry&&) = default;
        FilterEntry& operator=(const FilterEntry&) = default;
    };

    robin_hood::unordered_node_map<qgram_t, FilterEntry, ModuloHash> m_filter_table;
    size_t m_filter_num;
    MinCount<FilterEntry*> m_filter_min;
    sketch::CountMin<qgram_t> m_sketch;

    template<typename lzqgram_t>
    void sketch(lzqgram_t& c, const qgram_t pattern) {
        // if filter is not yet full, add pattern to filter
        // otherwise, count it in the sketch and check if maybe it needs to be swapped to the filter
        bool enter_into_filter;
        index_t count;
        
        if(m_filter_table.size() >= m_filter_size) {
            if constexpr(lzqgram_t::track_stats) ++c.stats().num_collisions;
            
            // count in sketch
            count = m_sketch.process_and_count(pattern);
            if(count > m_filter_min.min()) {
                // sketched count now exceeds filter minimum - we need to swap
                enter_into_filter = true;
                if constexpr(lzqgram_t::track_stats) ++c.stats().num_swaps;
                
                // extract minimum from filter
                FilterEntry* min = m_filter_min.extract_min();
                assert(m_filter_table.find(min->pattern) != m_filter_table.end());
                const auto delta = min->new_count - min->old_count;
                m_filter_table.erase(min->pattern);
                assert(m_filter_table.find(min->pattern) == m_filter_table.end());
                
                // "put it into sketch"
                if(delta > 0) {
                    m_sketch.process(pattern, delta);
                }
            } else {
                enter_into_filter = false;
            }
        } else {
            count = 1;
            enter_into_filter = true;
        }
        
        // enter into filter
        if(enter_into_filter) {
            auto result = m_filter_table.emplace(pattern, FilterEntry(pattern, c.pos(), count));
            if(result.second) {
                auto& filter_entry = result.first->second;
                auto min_entry = m_filter_min.insert(&filter_entry, count);
                assert(min_entry.item() == &filter_entry);
                filter_entry.min_ds_entry = min_entry;
            } else {
                std::cerr << "failed to insert pattern into filter" << std::endl;
                std::abort();
            }
        }
    }

public:
    SketchProcessor() : m_filter_table(m_filter_size), m_filter_num(0), m_sketch(m_sketch_cols, m_sketch_rows) {
    }

    template<typename lzqgram_t>
    void process(lzqgram_t& c, std::ostream& out) {
        size_t lpf = 0;
        size_t src = 0;
        
        size_t len = c.threshold();
        const size_t jmax = lzqgram_t::q - 1 - c.threshold();
        for(size_t j = 0; j < jmax; j++) {
            // get qgram prefix of length len
            const auto prefix = c.qgram_prefix(len);
            
            // test if prefix is contained in filter
            {
                auto e = m_filter_table.find(prefix);
                if(e != m_filter_table.end()) {
                    // prefix is in filter - store as current "longest previous factor"
                    auto& filter_entry = e->second;
                    
                    lpf = len;
                    src = filter_entry.seen_at;
                    
                    // update filter
                    filter_entry.seen_at = c.pos();
                    ++filter_entry.new_count;
                    m_filter_min.increment(filter_entry.min_ds_entry);
                } else {
                    // prefix is not in filter, sketch it
                    sketch(c, prefix);
                }
            }
            ++len;
        }
        
        c.output_ref(out, src, lpf);
    }
};

}}}} // namespace tdc::comp::lz77::qgram
