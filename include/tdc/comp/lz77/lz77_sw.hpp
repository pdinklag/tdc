#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <vector>

#include <divsufsort.h>
#include "lcp.hpp"

#include <tdc/util/index.hpp>
#include <tdc/util/literals.hpp>

#include "stats.hpp"

namespace tdc {
namespace comp {
namespace lz77 {

template<bool m_track_stats = false>
class LZ77SlidingWindow {
private:
    static constexpr bool verbose = false; // use for debugging
    
    using char_t = unsigned char;
    using window_index_t = uint32_t;
    
    static constexpr index_t NONE = 0;
    static constexpr index_t ROOT = 1;
    
    class CompactTrie {
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
        CompactTrie(const size_t label_bufsize, const size_t initial_capacity) {
            m_label_buffer = new char_t[label_bufsize];
            reserve(initial_capacity);
            clear();
        }
        
        ~CompactTrie() {
            delete[] m_label_buffer;
        }
        
        CompactTrie(const CompactTrie&) = default;
        CompactTrie(CompactTrie&&) = default;
        CompactTrie& operator=(const CompactTrie&) = default;
        CompactTrie& operator=(CompactTrie&&) = default;
        
        index_t emplace_back(const char_t c, const index_t label, const index_t llen) {
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
        
        void clear() {
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
        
        void reserve(const size_t cap) {
            m_parent.reserve(cap);
            m_first_child.reserve(cap);
            m_next_sibling.reserve(cap);
            
            m_char.reserve(cap);
            m_label.reserve(cap);
            m_llen.reserve(cap);
            m_min_pos.reserve(cap);
            m_max_pos.reserve(cap);
        }
        
        void write_label_buffer(const char_t* src, const size_t len) {
            for(size_t i = 0; i < len; i++) {
                m_label_buffer[i] = src[i];
            }
        }
        
        index_t& parent(const index_t node)       { return m_parent[node]; }
        index_t& first_child(const index_t node)  { return m_first_child[node]; }
        index_t& next_sibling(const index_t node) { return m_next_sibling[node]; }
        char_t& in_char(const index_t node)       { return m_char[node]; }
        window_index_t& label(const index_t node)        { return m_label[node]; }
        window_index_t& llen(const index_t node)         { return m_llen[node]; }
        window_index_t& min_pos(const index_t node)      { return m_min_pos[node]; }
        window_index_t& max_pos(const index_t node)      { return m_max_pos[node]; }
        
        const char_t* label_buffer(const size_t start = 0) { return m_label_buffer + start; }
        const char_t label_char(const index_t node, const size_t i = 0) { return *(label_buffer(m_label[node]) + i); }
        bool is_leaf(const index_t node) const { return m_first_child[node] == NONE; }
        
        index_t get_child(const index_t node, const char_t c) {
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
        
        void insert_child(const index_t node, const index_t child) {
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

        size_t size() const {
            return m_parent.size();
        }
    };
    
    struct CompactTrieCursor {
        CompactTrie* trie;
        index_t node;
        index_t lpos;
        index_t depth;
        
        CompactTrieCursor(CompactTrie& trie_, const index_t node_ = ROOT) : trie(&trie_), node(node_), lpos(0), depth(0) {
        }
        
        CompactTrieCursor(const CompactTrieCursor&) = default;
        CompactTrieCursor(CompactTrieCursor&&) = default;
        CompactTrieCursor& operator=(const CompactTrieCursor&) = default;
        CompactTrieCursor& operator=(CompactTrieCursor&&) = default;
        
        // get the last read character
        char_t character() const {
            assert(lpos > 0);
            return trie->label_char(node, lpos - 1);
        }
        
        // test if we reached a leaf
        bool reached_leaf() const {
            return lpos >= trie->llen(node) && trie->is_leaf(node);
        }
        
        // min position of an occurence of prefix at cursor
        window_index_t min_pos() const {
            return trie->min_pos(node);
        }
        
        // max position of an occurence of prefix at cursor
        window_index_t max_pos() const {
            return trie->max_pos(node);
        }
        
        // descend down with one character
        bool descend(const char_t c) {
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
        void ascend(const index_t d) {
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
        void insert_path(index_t s, index_t len) {
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
    
    // buffers are expected to be of size at least n+1 (zero-terminator)!
    void compute_sw_trie(CompactTrie* trie, const char_t* buffer, const index_t n, const index_t w, saidx_t* sa, saidx_t* lcp, saidx_t* work) {
        // compute suffix and LCP array
        //divsuflcpsort((const sauchar_t*)buffer, sa, lcp, (saidx_t)(n+1)); // nb: BROKEN??
        divsufsort((const sauchar_t*)buffer, sa, (saidx_t)(n+1));
        assert(sa[0] == n);
        assert(buffer[sa[0]] != buffer[sa[1]]); // must have unique terminator
        
        compute_lcp(buffer, n+1, sa, lcp, work);
        assert(lcp[0] == 0);
        assert(lcp[1] == 0);
        
        // init trie
        trie->clear();
        trie->write_label_buffer(buffer, n+1);
        
        // compute truncated suffix tree w/ min/max info
        trie->clear();
        CompactTrieCursor cursor(*trie);

        // first suffix begins with terminator, skip
        for(size_t i = 1; i < n + 1; i++) {
            // ascend to depth lcp[i]
            cursor.ascend(lcp[i]);
            assert(cursor.depth <= lcp[i]);
            // nb: Strictly speaking, in the classical suffix trie construction algorithm, it should be cursor.depth == lcp[i].
            //     However, we are not entering ALL suffixes, but only those starting in the window.
            //     Thus, we may have skipped some on the way.

            // we are only interested in suffixes starting in [0,w-1]
            const index_t pos = sa[i];
            if(pos < w) {
                const auto d = cursor.depth;
                assert(d <= w);
                
                // it may happen that the LCP of two suffixes is already larger than the window
                // in that case, there is nothing more to insert here
                if(d < w) {
                    // insert remainder of truncated suffix
                    const index_t suffix = pos + d;
                    const index_t suffix_len = std::min(n - (pos + d), w - d);
                
                    cursor.insert_path(suffix, suffix_len);
                    assert(cursor.depth == std::min(n - pos, w));
                }
                
                // propagate min and max position
                {
                    auto v = cursor.node;
                    while(v) {
                        trie->min_pos(v) = std::min(trie->min_pos(v), (window_index_t)(pos));
                        trie->max_pos(v) = std::max(trie->max_pos(v), (window_index_t)(pos));
                        v = trie->parent(v);
                    }
                }
            }
        }
        
        // done
        if constexpr(m_track_stats) m_stats.trie_size = std::max(m_stats.trie_size, trie->size());
        assert(trie->min_pos(ROOT) == 0);
        assert(trie->max_pos(ROOT) <= w-1);
    }
    
    index_t m_window, m_threshold;
    Stats m_stats;
    
public:
    LZ77SlidingWindow(const index_t window, const index_t threshold) : m_window(window), m_threshold(threshold) {
        if(m_window > std::numeric_limits<window_index_t>::max()) {
            throw std::runtime_error("maximum allowed window size exceeded");
        }
    }

    void compress(std::istream& in, std::ostream& out) {
        index_t window_offset = 0;
        
        const auto bufsize = 2 * m_window;
        char_t* buffer = new char_t[bufsize + 1];
        
        CompactTrie sw_tries[2] = { CompactTrie(bufsize + 1, 2 * m_window), CompactTrie(bufsize + 1, 2 * m_window) };
        CompactTrie* left_trie;
        CompactTrie* right_trie;
        bool cur_right_trie = 0; // we swap this for every trie we build
        
        saidx_t* sa_buffer = new saidx_t[bufsize + 1];
        saidx_t* lcp_buffer = new saidx_t[bufsize + 1];
        saidx_t* work_buffer = new saidx_t[bufsize + 1];
        
        size_t last_block_len = 0;
        
        // initialize
        size_t n = 0; // not yet known
        size_t i = 0;
        size_t b = 0;
        size_t window_start = 0;
        size_t prev_window_start = 0;
        
        // read initial 2w characters
        if(in) {
            left_trie = &sw_tries[0]; // empty
            in.read((char*)buffer, bufsize);
            const auto r = in.gcount();
            if(r > 0) {
                buffer[r] = (char_t)0; // terminate
                right_trie = &sw_tries[1];
                cur_right_trie = 1;
                compute_sw_trie(right_trie, buffer, r, m_window, sa_buffer, lcp_buffer, work_buffer);
                // if constexpr(verbose) right_trie->print();
                n = r;
                last_block_len = std::min((index_t)r, m_window);
            }
        }
        
        while(i < n || in) {
            if constexpr(verbose) std::cout << "i=" << i << std::endl;
            if(i / m_window > b) {
                // entering new block
                b = i / m_window;
                window_start = b * m_window;
                prev_window_start = window_start - m_window;
                if constexpr(verbose) std::cout << "entering block " << b << std::endl;
                
                // right trie now becomes left trie
                left_trie = right_trie;
                cur_right_trie = !cur_right_trie;
                right_trie = &sw_tries[cur_right_trie];
                
                // copy last w characters to beginning of buffer
                for(size_t i = 0; i < m_window; i++) {
                    buffer[i] = buffer[m_window + i];
                }
                
                // read next w characters and compute sliding window trie
                {
                    char_t* rbuffer = buffer + last_block_len;
                    
                    size_t r;
                    if(in) {
                        in.read((char*)rbuffer, m_window);
                        r = in.gcount();
                    } else {
                        r = 0;
                    }
                    buffer[last_block_len + r] = (char_t)0; // terminate
                     
                    if constexpr(verbose) std::cout << "(read " << r << " character(s) from stream: \"" << rbuffer << "\")" << std::endl;
                    compute_sw_trie(right_trie, buffer, last_block_len + r, m_window, sa_buffer, lcp_buffer, work_buffer);
                    n += r;
                    last_block_len = r;
                    
                    // if constexpr(verbose) left_trie->print();
                    // if constexpr(verbose) right_trie->print();
                }
            }
            
            // perform match starting at position i
            {
                CompactTrieCursor lv(*left_trie);
                bool lsearch = true;
                CompactTrieCursor rv(*right_trie);
                bool rsearch = true;

                size_t j = i;
                assert(j < n);
                char_t c;
                while(j < n && (lsearch || rsearch)) {
                    // get next character
                    c = buffer[j - b * m_window];
                    if constexpr(verbose) std::cout << "\tread \"" << c << "\"" << std::endl;
                    
                    // search in left trie
                    if(lsearch) {
                        auto lc = lv;
                        if(lc.descend(c)) {
                            if(prev_window_start + lc.max_pos() + m_window >= i) {
                                // follow edge
                                if constexpr(verbose) std::cout << "\t\tT: following edge with max(e)=" << (prev_window_start + lc.max_pos()) << " >= i-w" << std::endl;
                                lv = lc;
                                lsearch = !lv.reached_leaf();
                            } else {
                                // can't follow edge
                                if constexpr(verbose) std::cout << "\t\tT: not following edge with max(e)=" << (prev_window_start + lc.max_pos()) << " < i-w" << std::endl;
                                lsearch = false;
                            }
                        } else {
                            // no such edge
                            if constexpr(verbose) std::cout << "\t\tT: no edge to follow" << std::endl;
                            lsearch = false;
                        }
                    }
                    
                    // search in right trie
                    if(rsearch) {
                        auto rc = rv;
                        if(rc.descend(c)) {
                            if(window_start + rc.min_pos() < i) {
                                // follow edge
                                if constexpr(verbose) std::cout << "\t\tT': following edge with min(e)=" << (window_start + rc.min_pos()) << " < i" << std::endl;
                                rv = rc;
                                rsearch = !rv.reached_leaf();
                            } else {
                                // can't follow edge
                                if constexpr(verbose) std::cout << "\t\tT': not following edge with min(e)=" << (window_start + rc.min_pos()) << ">= i" << std::endl;
                                rsearch = false;
                            }
                        } else {
                            // no such edge
                            if constexpr(verbose) std::cout << "\t\tT': no edge to follow" << std::endl;
                            rsearch = false;
                        }
                    }
                    
                    // match next character
                    ++j;
                }
                
                // TODO: if a leaf was reached, continue matching!
                
                // conclude match
                if constexpr(verbose) std::cout << "search stopped at d(T)=" << lv.depth << " and d(T')=" << rv.depth << std::endl;
                const auto flen = std::max(lv.depth, rv.depth);
                if(flen > 0) {
                    if(flen > 1) {
                        // print reference
                        if constexpr(m_track_stats) ++m_stats.num_refs;
                        const auto fsrc = (lv.depth > rv.depth) ? (prev_window_start + lv.max_pos()) : (window_start + rv.min_pos());
                        out << "(" << fsrc << "," << flen << ")";
                        if constexpr(verbose) std::cout << "-> (" << fsrc << "," << flen << ")" << std::endl;
                    } else {
                        // don't bother printing a reference of length 1
                        if constexpr(m_track_stats) ++m_stats.num_literals;
                        const char_t x = (lv.depth > 0) ? lv.character() : rv.character();
                        out << x;
                        if constexpr(verbose) std::cout << "-> " << x << std::endl;
                    }
                    
                    i += flen;
                } else {
                    // print unmatched character
                    if constexpr(m_track_stats) ++m_stats.num_literals;
                    out << c;
                    if constexpr(verbose) std::cout << "-> " << c << std::endl;
                    ++i;
                }
            }
        }
        
        // clean up
        delete[] sa_buffer;
        delete[] lcp_buffer;
        delete[] work_buffer;
        delete[] buffer;
        
        // stats
        if constexpr(m_track_stats) m_stats.input_size = n;
    }

    const Stats& stats() const { return m_stats; }
};

}}} // namespace tdc::comp::lz77
