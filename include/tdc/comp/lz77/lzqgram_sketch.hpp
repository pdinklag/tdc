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
    class ModuloHash {
    public:
        size_t operator()(const packed_chars_t key) const {
            return (size_t)(key % m_filter_prime);
        }
    };
    
    static char_t extract_front(packed_chars_t packed) {
        constexpr size_t front_rsh = (pack_num - 1) * std::numeric_limits<char_t>::digits;
        return (char_t)(packed >> front_rsh);
    }
    
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
    
    size_t m_last_ref_src;
    size_t m_last_ref_len;
    size_t m_last_ref_src_end;
    
    size_t m_threshold;

    void reset() {
        m_pos = 0;
        m_next_factor = 0;
        
        m_filter_table.clear();
        m_filter_num = 0;
        m_buffer = 0;
        m_read = 0;
    }
    
    void process(char_t c, std::ostream& out) {
        // std::cout << "T[" << std::dec << m_pos << "] = " << c << " = 0x" << std::hex << size_t(c) << std::endl;
        const char_t front = extract_front(m_buffer);
        m_buffer = (m_buffer << char_bits) | c;
        ++m_read;
        
        if(m_read >= pack_num) {
            // read buffer contents
            // std::cout << "process buffer" << std::endl;
            auto prefix = m_buffer;
            size_t len = pack_num;
            for(size_t j = 0; j < pack_num - (m_threshold-1); j++) {
                // std::cout << "\tprefix 0x" << std::hex << prefix << std::endl;
                
                // test if prefix has been seen before
                size_t cur_h;
                if(m_pos >= m_next_factor) {
                    auto e = m_filter_table.find(prefix);
                    if(e != m_filter_table.end()) {
                        auto& filter_entry = e->second;
                        assert(filter_entry.pattern == prefix);
                        assert(filter_entry.min_ds_entry.valid());
                        assert(filter_entry.min_ds_entry.count() == filter_entry.new_count);
                        assert(filter_entry.min_ds_entry.item() == &filter_entry);
                        
                        const auto src = filter_entry.seen_at;
                        filter_entry.seen_at = m_pos;
                        ++filter_entry.new_count;
                        m_filter_min.increment(filter_entry.min_ds_entry);
                        
                        // std::cout << "\t\toutput reference (" << std::dec << m_table[i][h].seen_at << "," << len << ")" << std::endl;
                        if(src == m_last_ref_src + m_last_ref_len) {
                            // extend last reference
                            m_last_ref_len += len;          
                            if constexpr(m_track_stats) {
                                ++m_stats.num_extensions;
                                m_stats.extension_sum += len;
                            }
                        } else {
                            if(m_last_ref_src != SIZE_MAX) {
                                out << "(" << m_last_ref_src << "," << m_last_ref_len << ")";
                                if constexpr(m_track_stats) ++m_stats.num_refs;
                            }
                            
                            m_last_ref_src = src;
                            m_last_ref_len = len;
                        }
                        
                        m_next_factor = m_pos + len;
                        
                        break;
                    } else {
                        bool enter_into_filter;
                        index_t count;
                        if(m_filter_table.size() >= m_filter_size) {
                            if constexpr(m_track_stats) ++m_stats.num_collisions;
                            
                            // move into sketch!
                            count = m_sketch.process_and_count(prefix);
                            if(count > m_filter_min.min()) {
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
                                    m_sketch.process(prefix, delta);
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
                            auto result = m_filter_table.emplace(prefix, FilterEntry(prefix, m_pos, count));
                            if(result.second) {
                                auto& filter_entry = result.first->second;
                                auto min_entry = m_filter_min.insert(&filter_entry, count);
                                assert(min_entry.item() == &filter_entry);
                                filter_entry.min_ds_entry = min_entry;
                            } else {
                                std::abort();
                            }
                        }
                    }
                }
                
                prefix >>= std::numeric_limits<char_t>::digits;
                --len;
            }
            
            // advance buffer
            if(m_pos >= m_next_factor) {
                // output literal
                // std::cout << "\toutput literal " << front << std::endl;
                if(m_last_ref_src != SIZE_MAX) {
                    if constexpr(m_track_stats) ++m_stats.num_refs;
                    out << "(" << m_last_ref_src << "," << m_last_ref_len << ")";
                }
                m_last_ref_src = SIZE_MAX;
                
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
          m_last_ref_src(SIZE_MAX),
          m_last_ref_len(0) {
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
        
        if(m_last_ref_src != SIZE_MAX) {
            out << "(" << m_last_ref_src << "," << m_last_ref_len << ")";
            if constexpr(m_track_stats) ++m_stats.num_refs;
        }
        
        if constexpr(m_track_stats) m_stats.input_size = m_pos;
        if constexpr(m_track_stats) m_stats.trie_size = m_filter_table.size();
    }
    
    const Stats& stats() const { return m_stats; }
};
    
}}} // namespace tdc::comp::lz77