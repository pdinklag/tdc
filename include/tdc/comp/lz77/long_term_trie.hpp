#pragma once

#include <vector>

#include <tdc/util/index.hpp>

namespace tdc {
namespace comp {
namespace lz77 {

class LongTermTrie {
public:
    using char_t = unsigned char;

private:
    static constexpr index_t ROOT = 0;

    std::vector<char_t>  m_char;
    std::vector<index_t> m_first_child;
    std::vector<index_t> m_next_sibling;
    std::vector<index_t> m_last_seen_at;
    std::vector<index_t> m_count;

    inline index_t emplace_back(char_t c) {
        const size_t sz = size();
        m_char.emplace_back(c);
        m_first_child.emplace_back(ROOT);
        m_next_sibling.emplace_back(ROOT);
        m_last_seen_at.emplace_back(0);
        m_count.emplace_back(0);
        return (index_t)sz;
    }

public:
    inline LongTermTrie() {
        m_char.reserve(16);
        m_first_child.reserve(16);
        m_next_sibling.reserve(16);
        m_last_seen_at.reserve(16);
        m_count.reserve(16);
        
        emplace_back(0); // node 0 is the root
    }

    inline index_t root() const {
        return ROOT;
    }
    
    inline size_t size() const {
        return m_char.size();
    }
    
    inline index_t get_child(const index_t node, const char_t c) {
        const auto first_child = m_first_child[node];
        auto v = first_child;

        {
            // MTF
            auto prev_sibling = ROOT;
            while(v != ROOT && m_char[v] != c) {
                prev_sibling = v;
                v = m_next_sibling[v];
            }
            
            if(v != ROOT && v != first_child) {
                m_next_sibling[prev_sibling] = m_next_sibling[v];
                m_next_sibling[v] = first_child;
                m_first_child[node] = v;
            }
        }
        
        return v;
    }
    
    inline index_t last_seen_at(const index_t node) const { return m_last_seen_at[node]; }
    inline index_t count(const index_t node) const { return m_count[node]; }
    
    inline void visit(const index_t node, const index_t pos) {
        m_last_seen_at[node] = pos;
        ++m_count[node];
    }
    
    inline index_t insert_child(const index_t parent, const char_t c, const index_t pos) {
        auto new_child = emplace_back(c);
        m_next_sibling[new_child] = m_first_child[parent];
        m_first_child[parent] = new_child;
        return new_child;
    }
    
    struct Cursor {
        LongTermTrie* trie;
        index_t node;
        index_t depth;
        
        inline Cursor(LongTermTrie& trie_, const index_t node_ = ROOT) : trie(&trie_), node(node_), depth(0) {
        }
        
        Cursor(const Cursor&) = default;
        Cursor(Cursor&&) = default;
        Cursor& operator=(const Cursor&) = default;
        Cursor& operator=(Cursor&&) = default;
        
        inline bool descend(const char_t c) {
            const auto v = trie->get_child(node, c);
            if(v != ROOT) {
                node = v;
                ++depth;
                return true;
            } else {
                return false;
            }
        }
        
        inline void insert_path(const char_t* s, index_t len, const index_t pos) {
            // descend as far down as possible
            while(len && descend(*s)) {
                visit(pos);
                ++s;
                --len;                
            }
            
            // insert remaining
            while(len) {
                trie->insert_child(node, *s, pos);
                ++s;
                --len;
            }
        }
        
        inline void visit(const index_t pos) {
            trie->visit(node, pos);
        }
        
        inline index_t last_seen_at() const { return trie->last_seen_at(node); }
    };
    
    inline Cursor cursor() { return Cursor(*this); }
};

}}} // tdc::comp::lz77

