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

template<std::unsigned_integral char_t = unsigned char, bool m_track_stats = false>
class LZQGramTrie {
private:
    using count_t = index_t;

    struct Trie {
        robin_hood::unordered_map<char_t, std::unique_ptr<Trie>> children;
        
        count_t count;
        index_t last_seen_at;
        
        Trie() : count(0), last_seen_at(0) {
        }
        
        Trie* get_or_create_child(const char_t c) {
            auto it = children.find(c);
            if(it != children.end()) {
                return it->second.get();
            } else {
                Trie* child = new Trie();
                children.emplace(c, std::unique_ptr<Trie>(child));
                return child;
            }
        }
    };

    using buffer_t = tlx::RingBuffer<char_t>;

    Trie m_trie;
    buffer_t m_buffer;
    std::unique_ptr<Stats> m_stats;
    
    size_t m_q;
    size_t m_pos;
    size_t m_next_factor;
    
    size_t m_threshold;

    void reset() {
        m_pos = 0;
        m_next_factor = 0;
        
        m_buffer.clear();
        m_trie = Trie();
        
        if constexpr(m_track_stats) {
            m_stats = std::make_unique<Stats>();
        }
    }
    
    void process(char_t c, std::ostream& out) {
        // std::cout << "T[" << std::dec << m_pos << "] = " << c << " = 0x" << std::hex << size_t(c) << std::endl;
        m_buffer.push_back(c);
        
        if(m_buffer.size() == m_q) {
            // navigate trie
            Trie* trie = &m_trie;
            
            index_t prev_occ = 0;
            size_t ref_len = 0;
            
            size_t len = 1;
            for(size_t i = 0; i < m_q; i++) {
                trie = trie->get_or_create_child(m_buffer[i]);
                if(trie->count) {
                    // seen before
                    if(len >= m_threshold) {
                        prev_occ = trie->last_seen_at;
                        ref_len = len;
                    }
                } else {
                    if constexpr(m_track_stats) ++m_stats->debug;
                }

                // log occurence
                ++trie->count;
                trie->last_seen_at = m_pos;
                
                // next prefix
                ++len;
            }
            
            if(m_pos >= m_next_factor) {
                if(ref_len) {
                    // std::cout << "\t\toutput reference (" << std::dec << prev_occ << "," << ref_len << ")" << std::endl;
                    out << "(" << prev_occ << "," << ref_len << ")";
                    m_next_factor = m_pos + ref_len;
                    
                    if constexpr(m_track_stats) ++m_stats->num_refs;
                } else {
                    // output literal
                    // std::cout << "\toutput literal " << m_buffer.front() << std::endl;
                    out << m_buffer.front();
                    ++m_next_factor;
                    
                    if constexpr(m_track_stats) ++m_stats->num_literals;
                }
            }
            
            // advance buffer
            m_buffer.pop_front();
            
            // advance
            ++m_pos;
        }
    }

public:
    LZQGramTrie(size_t q, size_t threshold = 2) : m_buffer(q), m_q(q), m_threshold(threshold) {
    }

    void compress(std::istream& in, std::ostream& out) {
        reset();
        
        char_t c;
        while(in.read((char*)&c, sizeof(char_t))) {
            process(c, out);
        }
        
        // process remainder
        for(size_t i = 0; i < m_q - 1; i++) {
            process(0, out);
        }
        
        if constexpr(m_track_stats) m_stats->input_size = m_pos;
    }
    
    const Stats& stats() const {
        if constexpr(m_track_stats) {
            return *m_stats.get();
        } else {
            return Stats {};
        }
    }
};
    
}}} // namespace tdc::comp::lz77