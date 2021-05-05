#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <limits>
#include <sstream>

#include <divsufsort.h>
#include <robin_hood.h>

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
    
    // TODO: switch from pointer-based to array-based (first-child-next-sibling) representation
    struct CompactTrie {
        const char_t* label;
        index_t llen;
        CompactTrie* parent;
        robin_hood::unordered_flat_map<char_t, CompactTrie*> children;
        
        // auxiliary information
        index_t min_pos, max_pos;
        
        CompactTrie() : label(nullptr), llen(0), parent(nullptr), min_pos(std::numeric_limits<index_t>::max()), max_pos(0) {
        }
        
        ~CompactTrie() {
            for(auto it : children) {
                delete it.second;
            }
        }
        
        bool is_leaf() const {
            return children.size() == 0;
        }
        
        void print(const char_t in = '\0', const size_t level = 0) const {
            for(size_t i = 0; i < level; i++) std::cout << "    ";
            std::cout << "'" << in << "' -> ";
            std::cout << "(" << llen << ") \"";
            for(size_t i = 0; i < llen; i++) std::cout << label[i];
            std::cout << "\" [" << min_pos << "," << max_pos << "]";
            if(is_leaf()) std::cout << " (LEAF)";
            std::cout << std::endl;
            for(auto it : children) {
                it.second->print(it.first, level + 1);
            }
        }
    };
    
    struct CompactTrieCursor {
        CompactTrie* node;
        index_t lpos;
        index_t depth;
        
        CompactTrieCursor(CompactTrie& node_) : node(&node_), lpos(0), depth(0) {
        }
        
        CompactTrieCursor(const CompactTrieCursor&) = default;
        CompactTrieCursor(CompactTrieCursor&&) = default;
        CompactTrieCursor& operator=(const CompactTrieCursor&) = default;
        CompactTrieCursor& operator=(CompactTrieCursor&&) = default;
        
        // get the last read character
        char_t character() const {
            assert(lpos > 0);
            return node->label[lpos - 1];
        }
        
        // test if we reached a leaf
        bool reached_leaf() const {
            return lpos >= node->llen && node->is_leaf();
        }
        
        // descend down with one character
        bool descend(const char_t c) {
            assert(node);
            if(lpos < node->llen) {
                if(node->label[lpos] == c) {
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
                auto it = node->children.find(c);
                if(it != node->children.end()) {
                    assert(*it->second->label == c);
                    
                    // continue in child
                    node = it->second;
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
                assert(node);
                assert(depth >= lpos);
                
                const index_t distance = depth - d;
                if(distance <= lpos - 1) {
                    // desired depth is on the current edge
                    lpos -= distance;
                    depth -= distance;
                } else {
                    // ascend to parent node
                    depth -= lpos;
                    node = node->parent;
                    lpos = node->llen;
                }
            }
        }
        
        // insert a path at the current position in trie for the given string
        void insert_path(const char_t* s, index_t len) {
            assert(len > 0);
            
            // navigate according to string as far as possible
            while(len && descend(*s)) {
                ++s;
                --len;
            }
            
            if(len == 0) {
                return; // nothing to insert!
            }
            
            // split edge if necessary
            if(lpos < node->llen) {
                assert(lpos > 0 || !node->parent); // only in the root node can we ever insert anything at depth zero
                
                // split the current edge by creating a new inner node
                CompactTrie* inner_node = new CompactTrie();
                CompactTrie* parent = node->parent;
                
                inner_node->parent = parent;
                inner_node->label = node->label;
                inner_node->llen = lpos;
                
                if(parent) {
                    assert(parent->children.find(*inner_node->label)->second == node);
                    parent->children[*inner_node->label] = inner_node;
                }
                
                node->label += lpos;
                node->llen -= lpos;
                inner_node->children.emplace(*node->label, node);
                
                inner_node->min_pos = node->min_pos;
                inner_node->max_pos = node->max_pos;
                
                node->parent = inner_node;
                node = inner_node;
                assert(lpos == node->llen);
            }
            
            // append a new child
            assert(node->children.find(*s) == node->children.end());
            
            {
                CompactTrie* new_child = new CompactTrie();
                new_child->parent = node;
                new_child->label = s;
                new_child->llen = len;
                node->children.emplace(*s, new_child);
                
                node = new_child;
                lpos = len;
                depth += len;
            }
        }
    };
    
    static void compute_lcp(const char_t* buffer, const index_t n, const saidx_t* sa, saidx_t* lcp, saidx_t* work) {
        // compute phi array
        {
            for(size_t i = 1, prev = sa[0]; i < n; i++) {
                work[sa[i]] = prev;
                prev = sa[i];
            }
            work[sa[0]] = sa[n-1];
        }
        
        // compute PLCP array
        {
            for(size_t i = 0, l = 0; i < n - 1; ++i) {
                const auto phi_i = work[i];
                while(buffer[i + l] == buffer[phi_i + l]) ++l;
                work[i] = l;
                if(l) --l;
            }
        }
        
        // compute LCP array
        {
            lcp[0] = 0;
            for(size_t i = 1; i < n; i++) {
                lcp[i] = work[sa[i]];
            }
        }
    }
    
    // buffers are expected to be of size at least n+1 (zero-terminator)!
    static std::unique_ptr<CompactTrie> compute_sw_trie(const char_t* buffer, const index_t n, const index_t w, const size_t start, char_t* label_buffer, saidx_t* sa, saidx_t* lcp, saidx_t* work) {
        // compute suffix and LCP array
        //divsuflcpsort((const sauchar_t*)buffer, sa, lcp, (saidx_t)(n+1)); // nb: BROKEN??
        divsufsort((const sauchar_t*)buffer, sa, (saidx_t)(n+1));
        assert(sa[0] == n);
        assert(buffer[sa[0]] != buffer[sa[1]]); // must have unique terminator
        
        compute_lcp(buffer, n+1, sa, lcp, work);
        assert(lcp[0] == 0);
        assert(lcp[1] == 0);
        
        // copy buffer to label buffer
        for(size_t i = 0; i < n; i++) {
            label_buffer[i] = buffer[i];
        }
        label_buffer[n] = 0;
        
        // compute truncated suffix tree w/ min/max info
        CompactTrie* root = new CompactTrie();
        
        CompactTrieCursor cursor(*root);

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
                    const char_t* suffix = label_buffer + pos + d;
                    const index_t suffix_len = std::min(n - (pos + d), w - d);
                
                    cursor.insert_path(suffix, suffix_len);
                    assert(cursor.depth == std::min(n - pos, w));
                }
                
                // propagate min and max position
                {
                    CompactTrie* v = cursor.node;
                    while(v) {
                        v->min_pos = std::min(v->min_pos, start + pos);
                        v->max_pos = std::max(v->max_pos, start + pos);
                        v = v->parent;
                    }
                }
            }
        }
        
        // done
        assert(root->min_pos == start);
        assert(root->max_pos <= start+w-1);
        
        return std::unique_ptr<CompactTrie>(root);
    }
    
    index_t m_window, m_threshold;
    Stats m_stats;
    
public:
    LZ77SlidingWindow(const index_t window, const index_t threshold) : m_window(window), m_threshold(threshold) {
    }

    void compress(std::istream& in, std::ostream& out) {
        index_t window_offset = 0;
        std::unique_ptr<CompactTrie> left_trie, right_trie;
        
        const auto bufsize = 2 * m_window;
        char_t* buffer = new char_t[bufsize + 1];
        
        char_t* trie_label_buffer[2]; // trie labels must be kept in separate buffers to avoid invalidation
        trie_label_buffer[0] = new char_t[bufsize + 1];
        trie_label_buffer[1] = new char_t[bufsize + 1];
        bool cur_trie_label_buffer = 0; // we swap this for every trie we build
        
        saidx_t* sa_buffer = new saidx_t[bufsize + 1];
        saidx_t* lcp_buffer = new saidx_t[bufsize + 1];
        saidx_t* work_buffer = new saidx_t[bufsize + 1];
        
        size_t last_block_len = 0;
        
        // initialize
        size_t n = 0; // not yet known
        size_t i = 0;
        size_t b = 0;
        
        // read initial 2w characters
        if(in) {
            left_trie = std::make_unique<CompactTrie>(); // empty
            in.read((char*)buffer, bufsize);
            const auto r = in.gcount();
            if(r > 0) {
                buffer[r] = (char_t)0; // terminate
                right_trie = compute_sw_trie(buffer, r, m_window, 0, trie_label_buffer[cur_trie_label_buffer], sa_buffer, lcp_buffer, work_buffer);
                if constexpr(verbose) right_trie->print();
                n = r;
                last_block_len = std::min((index_t)r, m_window);
            }
        }
        
        while(i < n || in) {
            if constexpr(verbose) std::cout << "i=" << i << std::endl;
            if(i / m_window > b) {
                // entering new block
                b = i / m_window;
                if constexpr(verbose) std::cout << "entering block " << b << std::endl;
                
                // right trie now becomes left trie
                left_trie = std::move(right_trie);
                
                // copy last w characters to beginning of buffer
                for(size_t i = 0; i < m_window; i++) {
                    buffer[i] = buffer[m_window + i];
                }
                
                // read next w characters
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
                    cur_trie_label_buffer = !cur_trie_label_buffer; // swap trie label buffers
                    right_trie = compute_sw_trie(buffer, last_block_len + r, m_window, b * m_window, trie_label_buffer[cur_trie_label_buffer], sa_buffer, lcp_buffer, work_buffer);
                    n += r;
                    last_block_len = r;
                    
                    if constexpr(verbose) left_trie->print();
                    if constexpr(verbose) right_trie->print();
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
                            if(lc.node->max_pos + m_window >= i) {
                                // follow edge
                                if constexpr(verbose) std::cout << "\t\tT: following edge with max(e)=" << (lc.node->max_pos) << " >= i-w" << std::endl;
                                lv = lc;
                                lsearch = !lv.reached_leaf();
                            } else {
                                // can't follow edge
                                if constexpr(verbose) std::cout << "\t\tT: not following edge with max(e)=" << (lc.node->max_pos) << " < i-w" << std::endl;
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
                            if(rc.node->min_pos < i) {
                                // follow edge
                                if constexpr(verbose) std::cout << "\t\tT': following edge with min(e)=" << (rc.node->min_pos) << " < i" << std::endl;
                                rv = rc;
                                rsearch = !rv.reached_leaf();
                            } else {
                                // can't follow edge
                                if constexpr(verbose) std::cout << "\t\tT': not following edge with min(e)=" << (rc.node->min_pos) << ">= i" << std::endl;
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
                        const auto fsrc = (lv.depth > rv.depth) ? lv.node->max_pos : rv.node->min_pos;
                        out << "(" << (fsrc) << "," << flen << ")";
                        if constexpr(verbose) std::cout << "-> (" << (fsrc) << "," << flen << ")" << std::endl;
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
        delete[] trie_label_buffer[0];
        delete[] trie_label_buffer[1];
        
        // stats
        if constexpr(m_track_stats) m_stats.input_size = n;
    }

    const Stats& stats() const { return m_stats; }
};

}}} // namespace tdc::comp::lz77
