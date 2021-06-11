#pragma once

#include <bit>
#include <cstdint>
#include <concepts>
#include <iostream>
#include <limits>
#include <stdexcept>

#include <tdc/hash/function.hpp>
#include <tdc/math/prime.hpp>
#include <tdc/pred/dynamic/btree.hpp>
#include <tdc/pred/dynamic/btree/btree_min_observer.hpp>
#include <tdc/pred/dynamic/btree/sorted_array_node.hpp>
#include <tdc/sketch/count_min.hpp>
#include <tdc/util/index.hpp>
#include <tdc/util/min_count.hpp>

#include <robin_hood.h>

#include "qgram_hash.hpp"
#include "stats.hpp"

namespace tdc {
namespace comp {
namespace lz77 {
namespace qgram {

// sketch using B-Tree as a minimum data structure, does not require referential integrity
template<std::unsigned_integral qgram_t>
class SketchProcessor2 {
private:
    struct FilterEntry {
        qgram_t pattern;
        index_t seen_at;
        
        index_t old_count;
        index_t new_count;

        FilterEntry() : FilterEntry(0, 0, 0) {
        }
        
        FilterEntry(qgram_t _pattern, index_t _seen_at, index_t _count) : pattern(_pattern), seen_at(_seen_at), old_count(_count), new_count(_count) {
        }
        
        FilterEntry(FilterEntry&&) = default;
        FilterEntry(const FilterEntry&) = default;
        FilterEntry& operator=(FilterEntry&&) = default;
        FilterEntry& operator=(const FilterEntry&) = default;
    };
    
    struct BTreeEntry {
        index_t count;
        qgram_t pattern;
        
        constexpr BTreeEntry() : pattern(0), count(0) {
        }
        
        BTreeEntry(qgram_t _pattern, index_t _count) : pattern(_pattern), count(_count) {
        }
        
        BTreeEntry(BTreeEntry&&) = default;
        BTreeEntry(const BTreeEntry&) = default;
        BTreeEntry& operator=(BTreeEntry&&) = default;
        BTreeEntry& operator=(const BTreeEntry&) = default;
        
        auto operator<=>(const BTreeEntry& e) const = default;
        bool operator==(const BTreeEntry& e) const = default;
        bool operator!=(const BTreeEntry& e) const = default;
    }  __attribute__((__packed__));

    size_t m_filter_size;
    robin_hood::unordered_node_map<qgram_t, FilterEntry, QGramHash<qgram_t>> m_filter_table;
    size_t m_filter_num;
    
    using btree_t = pred::dynamic::BTree<BTreeEntry, 33, pred::dynamic::SortedArrayNode<BTreeEntry, 32>>;
    btree_t m_btree;
    pred::dynamic::BTreeMinObserver<btree_t> m_btree_min;
    
    //MinCount<FilterEntry*> m_filter_min;
    
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
            
            assert(m_btree_min.has_min());
            auto min = m_btree_min.min();
            if(count > min.count) {
                // sketched count now exceeds filter minimum - we need to swap
                enter_into_filter = true;
                if constexpr(lzqgram_t::track_stats) ++c.stats().num_swaps;
                
                // extract minimum from filter
                m_btree.remove(min);
                assert(m_filter_table.find(min.pattern) != m_filter_table.end());
                auto min_filter = m_filter_table.find(min.pattern)->second;
                
                const auto delta = min_filter.new_count - min_filter.old_count;
                m_filter_table.erase(min.pattern);
                assert(m_filter_table.find(min.pattern) == m_filter_table.end());
                assert(m_btree.size() == m_filter_table.size());
                
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
                m_btree.insert(BTreeEntry(pattern, count));
                assert(m_btree.size() == m_filter_table.size());
            } else {
                std::cerr << "failed to insert pattern into filter" << std::endl;
                std::abort();
            }
        }        
    }

public:
    SketchProcessor2(size_t filter_size, size_t sketch_cols, size_t sketch_rows)
        : m_filter_size(filter_size),
          m_filter_table(m_filter_size),
          m_filter_num(0),
          m_btree_min(m_btree),
          m_sketch(sketch_cols, sketch_rows) {

        m_btree.set_observer(&m_btree_min);
    }

    template<typename lzqgram_t>
    requires (lzqgram_t::qgram_endian == std::endian::little)
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

                    assert(m_btree.contains(BTreeEntry(prefix, filter_entry.new_count)));
                    m_btree.remove(BTreeEntry(prefix, filter_entry.new_count));
                    ++filter_entry.new_count;
                    m_btree.insert(BTreeEntry(prefix, filter_entry.new_count)); // TODO: increase key for B-Tree?
                    assert(m_btree.size() == m_filter_table.size());
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
