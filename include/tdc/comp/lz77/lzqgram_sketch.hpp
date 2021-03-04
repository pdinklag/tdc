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

template<size_t m_filter_size, std::unsigned_integral packed_chars_t = uint64_t, std::unsigned_integral char_t = unsigned char, bool m_track_stats = false>
class LZQGramSketch {
private:
    using count_t = index_t;

    static constexpr size_t pack_num = sizeof(packed_chars_t) / sizeof(char_t);
    static constexpr size_t char_bits = std::numeric_limits<char_t>::digits;
    static constexpr uint64_t m_filter_prime = math::prime_predecessor(m_filter_size);
    
    static constexpr size_t REF_INVALID = SIZE_MAX;
    
    class ModuloHash {
    public:
        size_t operator()(const packed_chars_t key) const {
            return (size_t)(key % m_filter_prime);
        }
    };
    
    struct FilterEntry {
        packed_chars_t pattern;
        index_t seen_at;
        
        index_t old_count;
        index_t new_count;
        
        MinCount<FilterEntry*>::Entry min_ds_entry;
        
        FilterEntry() : FilterEntry(0, 0, 0) {
        }
        
        FilterEntry(packed_chars_t _pattern, index_t _seen_at, index_t _count) : pattern(_pattern), seen_at(_seen_at), old_count(_count), new_count(_count) {
        }
        
        FilterEntry(FilterEntry&&) = default;
        FilterEntry(const FilterEntry&) = default;
        FilterEntry& operator=(FilterEntry&&) = default;
        FilterEntry& operator=(const FilterEntry&) = default;
    };

    using filter_table_t = robin_hood::unordered_node_map<packed_chars_t, FilterEntry, ModuloHash>;
    
    filter_table_t m_filter_table;
    size_t m_filter_num;
    MinCount<FilterEntry*> m_filter_min;
    sketch::CountMin<packed_chars_t> m_sketch;
    
    packed_chars_t m_buffer;
    size_t m_read;
    
    Stats m_stats;
    
    size_t m_pos;
    size_t m_next_factor;
    
    size_t m_cur_src;
    size_t m_cur_len;
    
    size_t m_threshold;

    void reset() {
        m_pos = 0;
        m_next_factor = 0;
        
        m_filter_table.clear();
        m_filter_num = 0;
        // TODO: min ds, sketch
        m_buffer = 0;
        m_read = 0;
        
        m_cur_src = REF_INVALID;
        m_cur_len = 0;
    }
    
    char_t buffer_front() {
        constexpr size_t front_rsh = (pack_num - 1) * char_bits;
        return (char_t)(m_buffer >> front_rsh);
    }
    
    void output_current_ref(std::ostream& out) {
        if(m_cur_src != REF_INVALID) {
            // std::cout << "\toutput reference (" << std::dec << m_cur_src << "," << m_cur_len << ")" << std::endl;
            out << "(" << m_cur_src << "," << m_cur_len << ")";
            if constexpr(m_track_stats) ++m_stats.num_refs;
        }
    }
    
    packed_chars_t buffer_prefix(const size_t len) const {
        return m_buffer >> (pack_num - len) * char_bits;
    }
    
    void sketch_pattern(const packed_chars_t pattern, const size_t pos) {
        // if filter is not yet full, add pattern to filter
        // otherwise, count it in the sketch and check if maybe it needs to be swapped to the filter
        bool enter_into_filter;
        index_t count;
        
        if(m_filter_table.size() >= m_filter_size) {
            if constexpr(m_track_stats) ++m_stats.num_collisions;
            
            // count in sketch
            count = m_sketch.process_and_count(pattern);
            if(count > m_filter_min.min()) {
                // sketched count now exceeds filter minimum - we need to swap
                enter_into_filter = true;
                if constexpr(m_track_stats) ++m_stats.num_swaps;
                
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
            auto result = m_filter_table.emplace(pattern, FilterEntry(pattern, pos, count));
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
    
    void process(char_t c, std::ostream& out) {
        m_buffer = (m_buffer << char_bits) | c;
        ++m_read;
        
        if(m_read >= pack_num) {
            const char_t front = buffer_front();
            
            size_t lpf = 0;
            size_t src = 0;
            
            size_t len = m_threshold;
            const size_t jmax = pack_num - 1 - m_threshold;
            for(size_t j = 0; j < jmax; j++) {
                // get buffer prefix of length len
                const auto prefix = buffer_prefix(len);
                
                // std::cout << "prefix 0x" << std::hex << prefix << std::endl;
                
                // test if prefix is contained in filter
                {
                    auto e = m_filter_table.find(prefix);
                    if(e != m_filter_table.end()) {
                        // prefix is in filter - store as current "longest previous factor"
                        auto& filter_entry = e->second;
                        
                        lpf = len;
                        src = filter_entry.seen_at;
                        
                        // update filter
                        filter_entry.seen_at = m_pos;
                        ++filter_entry.new_count;
                        m_filter_min.increment(filter_entry.min_ds_entry);
                    } else {
                        // prefix is not in filter, sketch it
                        sketch_pattern(prefix, m_pos);
                    }
                }
                ++len;
            }
            
            if(m_pos >= m_next_factor && lpf >= m_threshold) {
                if(src == m_cur_src + m_cur_len) {
                    // extend current reference
                    // std::cout << "\textend reference (" << std::dec << m_cur_src << "," << m_cur_len << ") to length " << (m_cur_len+len) << std::endl;
                    
                    m_cur_len += lpf;          
                    if constexpr(m_track_stats) {
                        ++m_stats.num_extensions;
                        m_stats.extension_sum += lpf;
                    }
                } else {
                    // output current reference
                    output_current_ref(out);
                    
                    // start new reference
                    m_cur_src = src;
                    m_cur_len = lpf;
                }
                
                m_next_factor = m_pos + lpf;
            }
            
            // advance buffer
            if(m_pos >= m_next_factor) {
                // output current reference, if any, and invalidate
                output_current_ref(out);
                m_cur_src = REF_INVALID;
                m_cur_len = 0;
                
                // output literal
                // std::cout << "\toutput literal " << front << std::endl;
                out << front;
                ++m_next_factor;
                
                if constexpr(m_track_stats) ++m_stats.num_literals;
            }
            
            // advance
            ++m_pos;
        }
    }

public:
    LZQGramSketch(size_t sketch_cols, size_t sketch_rows, size_t threshold = 2)
        : m_buffer(0),
          m_read(0),
          m_filter_table(m_filter_size),
          m_filter_num(0),
          m_sketch(sketch_cols, sketch_rows),
          m_threshold(threshold),
          m_cur_src(REF_INVALID),
          m_cur_len(0) {
    }

    void compress(std::istream& in, std::ostream& out) {
        reset();
        
        constexpr size_t bufsize = 1_Mi;
        char_t* buffer = new char_t[bufsize];
        bool b;
        do {
            b = (bool)in.read((char*)buffer, bufsize * sizeof(char_t));
            const size_t num = in.gcount() / sizeof(char_t);
            for(size_t i = 0; i < num; i++) {
                process(buffer[i], out);
            }
        } while(b);
        delete[] buffer;
        
        // process remainder
        for(size_t i = 0; i < pack_num - 1; i++) {
            process(0, out);
        }
        
        // output last reference, if any
        output_current_ref(out);
        
        if constexpr(m_track_stats) m_stats.input_size = m_pos;
        if constexpr(m_track_stats) m_stats.trie_size = m_filter_table.size();
    }
    
    const Stats& stats() const { return m_stats; }
};
    
}}} // namespace tdc::comp::lz77