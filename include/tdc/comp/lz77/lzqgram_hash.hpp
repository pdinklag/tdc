#pragma once

#include <cstdint>
#include <concepts>
#include <iostream>
#include <limits>

#include <robin_hood.h>
#include <tlx/container/ring_buffer.hpp>

#include <tdc/util/index.hpp>

#include "stats.hpp"

namespace tdc {
namespace comp {
namespace lz77 {

template<std::unsigned_integral char_t = unsigned char, std::unsigned_integral packed_chars_t = uint64_t, bool m_track_stats = false>
class LZQGramHash {
private:
    using buffer_t = tlx::RingBuffer<char_t>;
    using count_t = index_t;
    
    struct FilterEntry {
        count_t count;
        index_t last_seen_at;

        FilterEntry() {
        }

        FilterEntry(count_t _count, index_t seen_at) : count(_count), last_seen_at(seen_at) {
        }
    };
    
    using filter_t = robin_hood::unordered_map<packed_chars_t, FilterEntry>;
    
    static constexpr size_t pack_num = sizeof(packed_chars_t) / sizeof(char_t);
    static packed_chars_t pack(const buffer_t& buffer) {
        packed_chars_t packed = buffer[0];
        for(size_t i = 1; i < pack_num; i++) {
            packed <<= std::numeric_limits<char_t>::digits;
            packed |= buffer[i];
        }
        return packed;
    }
    
    filter_t m_filter;
    buffer_t m_buffer;
    Stats m_stats;
    
    size_t m_pos;
    size_t m_next_factor;
    
    size_t m_threshold;

    void reset() {
        m_pos = 0;
        m_next_factor = 0;
        
        m_buffer.clear();
        m_filter.clear();
        
        if constexpr(m_track_stats) {
            m_stats = std::make_unique<Stats>();
        }
    }
    
    void process(char_t c, std::ostream& out) {
        // std::cout << "T[" << std::dec << m_pos << "] = " << c << " = 0x" << std::hex << size_t(c) << std::endl;
        m_buffer.push_back(c);
        
        if(m_buffer.size() == pack_num) {
            // read buffer contents
            // std::cout << "process buffer" << std::endl;
            auto prefix = pack(m_buffer);
            size_t len = pack_num;
            for(size_t i = 0; i < pack_num - (m_threshold-1); i++) {
                // std::cout << "\tprefix 0x" << std::hex << prefix << std::endl;
                
                // test if prefix has been seen before
                if(m_filter.find(prefix) != m_filter.end()) {
                    auto& e = m_filter[prefix];
                    
                    if(m_pos >= m_next_factor) {
                        // output reference
                        // std::cout << "\t\toutput reference (" << std::dec << e.last_seen_at << "," << len << std::endl;
                        out << "(" << e.last_seen_at << "," << len << ")";
                        m_next_factor = m_pos + len;
                        
                        if constexpr(m_track_stats) ++m_stats.num_refs;
                    }
                    
                    e.count++;
                    e.last_seen_at = m_pos;
                } else {
                    m_filter.emplace(prefix, FilterEntry(1, m_pos));
                }
                
                prefix >>= std::numeric_limits<char_t>::digits;
                --len;
            }
            
            // advance buffer
            if(m_pos >= m_next_factor) {
                // output literal
                // std::cout << "\toutput literal " << m_buffer.front() << std::endl;
                out << m_buffer.front();
                ++m_next_factor;
                
                if constexpr(m_track_stats) ++m_stats.num_literals;
            }
            m_buffer.pop_front();
            
            // advance
            ++m_pos;
        }
    }

public:
    LZQGramHash(size_t threshold = 2) : m_buffer(pack_num), m_threshold(threshold) {
    }

    void compress(std::istream& in, std::ostream& out) {
        reset();
        
        char_t c;
        while(in.read((char*)&c, sizeof(char_t))) {
            process(c, out);
        }
        
        // process remainder
        for(size_t i = 0; i < pack_num - 1; i++) {
            process(0, out);
        }
        
        if constexpr(m_track_stats) m_stats.input_size = m_pos;
        if constexpr(m_track_stats) m_stats.trie_size = m_filter.size();
    }
    
    const Stats& stats() const { return m_stats; }
};
    
}}} // namespace tdc::comp::lz77