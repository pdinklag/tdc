#pragma once

#include <cassert>
#include <vector>

#include <divsufsort.h>

#include <tdc/util/lcp.hpp>
#include <tdc/util/index.hpp>

namespace tdc {
namespace comp {
namespace lz77 {

using window_index_t = uint32_t;

class SlidingWindowTrie {
public:
    using char_t = unsigned char;
    
    static constexpr index_t NONE = 0;
    static constexpr index_t ROOT = 1;

private:
    char_t* m_label_buffer;

    std::vector<index_t> m_parent;
    std::vector<index_t> m_first_child;
    std::vector<index_t> m_next_sibling;
    
    std::vector<char_t>  m_char;    // first character on incoming edge
    std::vector<window_index_t> m_label;   // full edge label
    std::vector<window_index_t> m_llen;    // label length
    std::vector<window_index_t> m_min_pos; // earliest occurrence
    std::vector<window_index_t> m_max_pos; // latest occurrence
    
public:
    inline SlidingWindowTrie(const size_t label_bufsize, const size_t initial_capacity) {
        m_label_buffer = new char_t[label_bufsize];
        reserve(initial_capacity);
        clear();
    }
    
    inline  ~SlidingWindowTrie() {
        delete[] m_label_buffer;
    }
    
    SlidingWindowTrie(const SlidingWindowTrie&) = default;
    SlidingWindowTrie(SlidingWindowTrie&&) = default;
    SlidingWindowTrie& operator=(const SlidingWindowTrie&) = default;
    SlidingWindowTrie& operator=(SlidingWindowTrie&&) = default;
    
    inline index_t emplace_back(const char_t c, const index_t label, const index_t llen) {
        auto idx = size();
        
        m_parent.emplace_back(NONE);
        m_first_child.emplace_back(NONE);
        m_next_sibling.emplace_back(NONE);
        
        m_char.emplace_back(c);
        m_label.emplace_back(label);
        m_llen.emplace_back(llen);
        m_min_pos.emplace_back(std::numeric_limits<index_t>::max());
        m_max_pos.emplace_back(0);
        return idx;
    }
    
    inline void clear() {
        m_parent.clear();
        m_first_child.clear();
        m_next_sibling.clear();
        
        m_char.clear();
        m_label.clear();
        m_llen.clear();
        m_min_pos.clear();
        m_max_pos.clear();
        
        emplace_back(0, -1, 0); // invalid node
        emplace_back(0, -1, 0); // root
    }
    
    inline void reserve(const size_t cap) {
        m_parent.reserve(cap);
        m_first_child.reserve(cap);
        m_next_sibling.reserve(cap);
        
        m_char.reserve(cap);
        m_label.reserve(cap);
        m_llen.reserve(cap);
        m_min_pos.reserve(cap);
        m_max_pos.reserve(cap);
    }
    
    inline index_t& parent(const index_t node)       { return m_parent[node]; }
    inline index_t& first_child(const index_t node)  { return m_first_child[node]; }
    inline index_t& next_sibling(const index_t node) { return m_next_sibling[node]; }
    inline char_t& in_char(const index_t node)       { return m_char[node]; }
    inline window_index_t& label(const index_t node)        { return m_label[node]; }
    inline window_index_t& llen(const index_t node)         { return m_llen[node]; }
    inline window_index_t& min_pos(const index_t node)      { return m_min_pos[node]; }
    inline window_index_t& max_pos(const index_t node)      { return m_max_pos[node]; }
    
    inline const char_t* label_buffer(const size_t start = 0) { return m_label_buffer + start; }
    inline const char_t label_char(const index_t node, const size_t i = 0) { return *(label_buffer(m_label[node]) + i); }
    inline bool is_leaf(const index_t node) const { return m_first_child[node] == NONE; }
    
    inline index_t get_child(const index_t node, const char_t c) {
        const auto first_child = m_first_child[node];
        auto v = first_child;

        auto prev_sibling = NONE;
        while(v && m_char[v] != c) {
            prev_sibling = v;
            v = m_next_sibling[v];
        }
        
        // MTF
        if(v && v != first_child) {
            m_next_sibling[prev_sibling] = m_next_sibling[v];
            m_next_sibling[v] = first_child;
            m_first_child[node] = v;
        }

        return v;
    }
    
    inline void insert_child(const index_t node, const index_t child) {
        if(m_parent[child]) {
            // remove child from current parent first
            auto old_parent = m_parent[child];
            auto v = m_first_child[old_parent];
            if(v == child) {
                // child is parent's first child, simply exchange
                m_first_child[old_parent] = m_next_sibling[child];
            } else {
                // search for previous sibling
                while(m_next_sibling[v] != child) {
                    v = m_next_sibling[v];
                    assert(v); // if we reach the end, the parent/child relationship was invalid
                }
                
                // unlink
                m_next_sibling[v] = m_next_sibling[child];
            }
        }
        
        m_next_sibling[child] = m_first_child[node];
        m_first_child[node] = child;
        m_parent[child] = node;
    }

    inline size_t size() const {
        return m_parent.size();
    }
    
    struct Cursor {
        SlidingWindowTrie* trie;
        index_t node;
        index_t lpos;
        index_t depth;
        
        inline Cursor(SlidingWindowTrie& trie_, const index_t node_ = ROOT) : trie(&trie_), node(node_), lpos(0), depth(0) {
        }
        
        Cursor(const Cursor&) = default;
        Cursor(Cursor&&) = default;
        Cursor& operator=(const Cursor&) = default;
        Cursor& operator=(Cursor&&) = default;
        
        // get the last read character
        inline char_t character() const {
            assert(lpos > 0);
            return trie->label_char(node, lpos - 1);
        }
        
        // test if we reached a leaf
        inline bool reached_leaf() const {
            return lpos >= trie->llen(node) && trie->is_leaf(node);
        }
        
        // min position of an occurence of prefix at cursor
        inline window_index_t min_pos() const {
            return trie->min_pos(node);
        }
        
        // max position of an occurence of prefix at cursor
        inline window_index_t max_pos() const {
            return trie->max_pos(node);
        }
        
        // descend down with one character
        inline bool descend(const char_t c) {
            if(lpos < trie->llen(node)) {
                if(trie->label_char(node, lpos) == c) {
                    // success
                    ++lpos;
                    ++depth;
                    return true;
                } else {
                    // character does not match the next one on the label
                    return false;
                }
            } else {
                // navigate to child
                auto v = trie->get_child(node, c);
                if(v) {
                    assert(trie->label_char(v) == c);
                    
                    // continue in child
                    node = v;
                    lpos = 0;
                    return descend(c);
                } else {
                    // no such child
                    return false;
                }
            }
        }
        
        // ascend up the trie to depth exactly d
        inline void ascend(const index_t d) {
            while(depth > d) {
                assert(depth >= lpos);
                
                const index_t distance = depth - d;
                if(distance <= lpos - 1) {
                    // desired depth is on the current edge
                    lpos -= distance;
                    depth -= distance;
                } else {
                    // ascend to parent node
                    depth -= lpos;
                    node = trie->parent(node);
                    lpos = trie->llen(node);
                }
            }
        }
        
        // insert a path at the current position in trie for the given string
        inline void insert_path(index_t s, index_t len) {
            assert(len > 0);
            
            // navigate according to string as far as possible
            while(len && descend(*trie->label_buffer(s))) {
                ++s;
                --len;
            }
            
            if(len == 0) {
                return; // nothing to insert!
            }
            
            // split edge if necessary
            if(lpos < trie->llen(node)) {
                assert(lpos > 0 || !trie->parent(node)); // only in the root node can we ever insert anything at depth zero
                
                // split the current edge by creating a new inner node
                auto inner_node = trie->emplace_back(trie->in_char(node), trie->label(node), lpos);
                
                auto parent = trie->parent(node);
                if(parent) {
                    assert(trie->get_child(parent, trie->in_char(inner_node)) == node);
                    trie->insert_child(parent, inner_node);
                }
                
                trie->label(node) += lpos;
                trie->llen(node)  -= lpos;
                assert(trie->llen(node) > 0);
                trie->in_char(node) = trie->label_char(node);
                
                trie->min_pos(inner_node) = trie->min_pos(node);
                trie->max_pos(inner_node) = trie->max_pos(node);
                
                trie->insert_child(inner_node, node);
                
                node = inner_node;
                assert(lpos == trie->llen(node));
            }
            
            // append a new child
            assert(trie->get_child(node, *trie->label_buffer(s)) == NONE);
            
            {
                auto new_child = trie->emplace_back(*trie->label_buffer(s), s, len);
                trie->insert_child(node, new_child);

                node = new_child;
                lpos = len;
                depth += len;
            }
        }
    };
    
    inline Cursor cursor() { return Cursor(*this); }
    
    // buffers are expected to be of size at least n+1 (zero-terminator)!
    void build(const char_t* buffer, const index_t n, const index_t window, saidx_t* sa, saidx_t* lcp, saidx_t* work) {
        // compute suffix and LCP array
        //divsuflcpsort((const sauchar_t*)buffer, sa, lcp, (saidx_t)(n+1)); // nb: BROKEN??
        divsufsort((const sauchar_t*)buffer, sa, (saidx_t)(n+1));
        assert(sa[0] == n);
        assert(buffer[sa[0]] != buffer[sa[1]]); // must have unique terminator
        
        lcp_kasai(buffer, n+1, sa, lcp, work);
        assert(lcp[0] == 0);
        assert(lcp[1] == 0);
        
        // init trie
        clear();
        
        // copy label buffer
        for(size_t i = 0; i < n+1; i++) {
            m_label_buffer[i] = buffer[i];
        }
        
        // compute truncated suffix tree w/ min/max info
        auto cur = cursor();

        // first suffix begins with terminator, skip
        for(size_t i = 1; i < n + 1; i++) {
            // ascend to depth lcp[i]
            cur.ascend(lcp[i]);
            assert(cur.depth <= lcp[i]);
            // nb: Strictly speaking, in the classical suffix trie construction algorithm, it should be cur.depth == lcp[i].
            //     However, we are not entering ALL suffixes, but only those starting in the window.
            //     Thus, we may have skipped some on the way.

            // we are only interested in suffixes starting in [0,w-1]
            const index_t pos = sa[i];
            if(pos < window) {
                const auto d = cur.depth;
                assert(d <= window);
                
                // it may happen that the LCP of two suffixes is already larger than the window
                // in that case, there is nothing more to insert here
                if(d < window) {
                    // insert remainder of truncated suffix
                    const index_t suffix = pos + d;
                    const index_t suffix_len = std::min(n - (pos + d), window - d);
                
                    cur.insert_path(suffix, suffix_len);
                    assert(cur.depth == std::min(n - pos, window));
                }
                
                // propagate min and max position
                {
                    auto v = cur.node;
                    while(v) {
                        min_pos(v) = std::min(min_pos(v), (window_index_t)(pos));
                        max_pos(v) = std::max(max_pos(v), (window_index_t)(pos));
                        v = this->parent(v);
                    }
                }
            }
        }
        
        // done
        assert(min_pos(ROOT) == 0);
        assert(max_pos(ROOT) <= window-1);
    }
};

}}} // namespace tdc::comp::lz77
