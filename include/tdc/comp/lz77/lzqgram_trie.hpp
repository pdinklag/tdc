#pragma once

#include <bit>
#include <cstdint>
#include <concepts>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>

#include <tdc/util/index.hpp>

#include "stats.hpp"

namespace tdc {
namespace comp {
namespace lz77 {
namespace qgram {

template<typename char_t, bool m_mtf>
class TrieProcessor {
private:
    static constexpr index_t ROOT = 0;

    std::vector<char_t>  m_char;
    std::vector<index_t> m_first_child;
    std::vector<index_t> m_next_sibling;
    std::vector<index_t> m_count;
    std::vector<index_t> m_last_seen_at;
    std::vector<index_t> m_shortcut;
    
    index_t m_last;
    
    index_t get_or_create_child(const index_t node, const char_t c, const index_t at) {
        const auto first_child = m_first_child[node];
        auto v = first_child;

        if constexpr(m_mtf) {
            // MTF
            auto prev_sibling = ROOT;
            while(v != ROOT && m_char[v] != c) {
                prev_sibling = v;
                v = m_next_sibling[v];
            }
            
            if(v && v != first_child) {
                m_next_sibling[prev_sibling] = m_next_sibling[v];
                m_next_sibling[v] = first_child;
                m_first_child[node] = v;
            }
        } else {
            // FIFO
            while(v != ROOT && m_char[v] != c) {
                v = m_next_sibling[v];
            }
        }
        
        if(v == ROOT) {
            // child does not exist, create new
            v = m_char.size();
            m_char.emplace_back(c);
            m_first_child.emplace_back(ROOT);
            m_next_sibling.emplace_back(first_child);
            m_count.emplace_back(0);
            m_last_seen_at.emplace_back(0);
            m_shortcut.emplace_back(ROOT);
            
            // insert
            m_first_child[node] = v;
        }
        return v;
    }

public:
    TrieProcessor() : m_last(ROOT) {
        m_char.reserve(16);
        m_first_child.reserve(16);
        m_next_sibling.reserve(16);
        m_count.reserve(16);
        m_last_seen_at.reserve(16);
        m_shortcut.reserve(16);
        
        // insert root
        m_char.emplace_back(0);
        m_first_child.emplace_back(ROOT);
        m_next_sibling.emplace_back(ROOT);
        m_count.emplace_back(0);
        m_last_seen_at.emplace_back(0);
        m_shortcut.emplace_back(ROOT);
    }

    template<typename lzqgram_t>
    void process(lzqgram_t& c, std::ostream& out) {
        // advance in trie as much as possible
        const auto _qgram = c.qgram();
        const char_t* qgram = (const char_t*)&_qgram;
        
        size_t len = 0;
        size_t src = 0;
        
        auto current = m_shortcut[m_last];
        if(current) {
            // shortcut existed
            if constexpr(lzqgram_t::track_stats) ++c.stats().num_collisions;
            
            len = lzqgram_t::q - 1;
            src = m_last_seen_at[current];
            ++m_count[current];
        }
        
        auto parent = current;
        for(size_t j = len; j < lzqgram_t::q; j++) {
            const size_t i = (lzqgram_t::qgram_endian == std::endian::little) ? lzqgram_t::q - 1 - j : j;
            
            parent = current;
            current = get_or_create_child(current, qgram[i], c.pos());
            if(m_count[current]) {
                ++len; // nb: this assumes that if a q-gram does not exist in the trie, neither does any (q+1)-gram extending it
                src = m_last_seen_at[current];
            } else {
                // node was just created
                if constexpr(lzqgram_t::track_stats) ++c.stats().trie_size;
            }
            
            // update
            ++m_count[current];
            m_last_seen_at[current] = c.pos();
        }
        
        if(m_last) m_shortcut[m_last] = parent;
        m_last = current;
        
        c.output_ref(out, src, len);
    }
};

}}}} // namespace tdc::comp::lz77::qgram