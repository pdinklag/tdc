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
    
    static constexpr size_t REF_INVALID = SIZE_MAX;
    
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
    
    size_t m_cur_src;
    size_t m_cur_len;
    
    size_t m_threshold;

    void reset() {
        m_pos = 0;
        m_next_factor = 0;
        
        m_filter = std::vector<FilterEntry>(m_filter_size, FilterEntry());
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
    
    void process(char_t c, std::ostream& out) {
        m_buffer = (m_buffer << char_bits) | c;
        ++m_read;
        
        if(m_read >= pack_num) {
            const char_t front = buffer_front();
            
            // read buffer contents
            auto prefix = m_buffer;
            size_t len = pack_num;
            for(size_t j = 0; j < pack_num - (m_threshold-1); j++) {
                // std::cout << "prefix 0x" << std::hex << prefix << std::endl;
                
                // test if prefix has been seen before
                const auto h = hash(prefix);
                if(m_filter[h].last == prefix) {
                    const auto src = m_filter[h].seen_at;
                    m_filter[h].seen_at = m_pos;
                    
                    if(m_pos >= m_next_factor) {
                        if(src == m_cur_src + m_cur_len) {
                            // extend current reference
                            // std::cout << "\textend reference (" << std::dec << m_cur_src << "," << m_cur_len << ") to length " << (m_cur_len+len) << std::endl;
                            
                            m_cur_len += len;          
                            if constexpr(m_track_stats) {
                                ++m_stats.num_extensions;
                                m_stats.extension_sum += len;
                            }
                        } else {
                            // output current reference
                            output_current_ref(out);
                            
                            // start new reference
                            m_cur_src = src;
                            m_cur_len = len;
                        }
                        
                        m_next_factor = m_pos + len;
                    }
                } else {
                    if constexpr(m_track_stats) {
                        if(m_filter[h].last) {
                            ++m_stats.num_collisions;
                        } else {
                            ++m_stats.trie_size;
                        }
                    }
                    m_filter[h].last = prefix;
                    m_filter[h].seen_at = m_pos;
                }
                
                prefix >>= char_bits;
                --len;
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
    LZQGramHash(size_t threshold = 2)
        : m_buffer(0),
          m_read(0),
          m_threshold(threshold),
          m_cur_src(REF_INVALID) {
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
    }
    
    const Stats& stats() const { return m_stats; }
};
    
}}} // namespace tdc::comp::lz77