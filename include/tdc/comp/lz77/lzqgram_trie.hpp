#pragma once

#include <bit>
#include <cstdint>
#include <concepts>
#include <iostream>
#include <limits>
#include <stdexcept>

#include <tdc/util/index.hpp>

#include "tries.hpp"
#include "stats.hpp"

namespace tdc {
namespace comp {
namespace lz77 {

template<std::unsigned_integral packed_chars_t = uint64_t, std::unsigned_integral char_t = unsigned char, Trie<char_t> trie_t = TrieList<char_t>, bool m_track_stats = false>
class LZQGramTrie {
private:
    using count_t = index_t;

    static constexpr size_t pack_num = sizeof(packed_chars_t) / sizeof(char_t);
    static constexpr size_t char_bits = std::numeric_limits<char_t>::digits;
    
    static constexpr size_t REF_INVALID = SIZE_MAX;
    
    trie_t m_trie;
    
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
        
        m_trie = trie_t();
        m_buffer = 0;
        m_read = 0;
        
        m_cur_src = REF_INVALID;
        m_cur_len = 0;
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
            // advance in trie as much as possible
            const char_t* buffer_chars = (const char_t*)&m_buffer;
            const char_t front = buffer_chars[0];
            
            size_t len = 0;
            size_t src = 0;
            
            auto* trie = &m_trie;
            for(size_t j = 0; j < pack_num; j++) {
                const size_t i = pack_num - 1 - j;
                trie = trie->get_or_create_child(buffer_chars[i]);
                if(trie->count()) {
                    ++len;
                    src = trie->last_seen_at();
                } else {
                    if constexpr(m_track_stats) ++m_stats.trie_size;
                }
                
                trie->update(trie->count() + 1, m_pos);
            }
            
            if(m_pos >= m_next_factor && len >= m_threshold) {
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
    LZQGramTrie(size_t threshold = 2)
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