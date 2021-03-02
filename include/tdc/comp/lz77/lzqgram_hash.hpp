#pragma once

#include <bit>
#include <cstdint>
#include <concepts>
#include <iostream>
#include <limits>
#include <stdexcept>

#include <tdc/math/prime.hpp>
#include <tdc/util/index.hpp>

#include "stats.hpp"

namespace tdc {
namespace comp {
namespace lz77 {

template<size_t m_filter_size, std::unsigned_integral packed_chars_t = uint64_t, std::unsigned_integral char_t = unsigned char, bool m_track_stats = false>
class LZQGramHash {
private:
    using count_t = index_t;

    static constexpr size_t pack_num = sizeof(packed_chars_t) / sizeof(char_t);
    static constexpr size_t char_bits = std::numeric_limits<char_t>::digits;
    static constexpr uint64_t m_filter_prime = math::prime_predecessor(m_filter_size);
    
    static char_t extract_front(packed_chars_t packed) {
        constexpr size_t front_rsh = (pack_num - 1) * std::numeric_limits<char_t>::digits;
        return (char_t)(packed >> front_rsh);
    }
    
    struct FilterEntry {
        packed_chars_t last;
        index_t seen_at;
        
        FilterEntry() : last(0), seen_at(0) {
        }
    } __attribute__((__packed__));

    std::vector<FilterEntry> m_filter;
    
    uint64_t hash(const packed_chars_t& key) const {
        // std::cout << "\t\th" << std::dec << i << "(0x" << std::hex << key << ") = 0x" << h << std::endl;
        return (uint64_t)((key % m_filter_prime) % m_filter_size);
    }
    
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
        
        m_filter = std::vector<FilterEntry>(m_filter_size, FilterEntry());
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
                    const auto h = hash(prefix);
                    if(m_filter[h].last == prefix) {
                        const auto src = m_filter[h].seen_at;
                        m_filter[h].seen_at = m_pos;
                        
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
                        if(m_filter[h].last) ++m_stats.num_collisions;
                        m_filter[h].last = prefix;
                        m_filter[h].seen_at = m_pos;
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
    LZQGramHash(size_t threshold = 2)
        : m_buffer(0),
          m_read(0),
          m_threshold(threshold),
          m_last_ref_src(SIZE_MAX) {
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
    }
    
    const Stats& stats() const { return m_stats; }
};
    
}}} // namespace tdc::comp::lz77