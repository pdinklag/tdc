#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <vector>

#include <tdc/util/index.hpp>
#include <tdc/util/literals.hpp>

#include "sliding_window_trie.hpp"
#include "stats.hpp"

namespace tdc {
namespace comp {
namespace lz77 {

template<bool m_allow_ext_match = false, bool m_track_stats = false>
class LZ77SlidingWindow {
private:
    static constexpr bool verbose = false; // use for debugging
    
    using char_t = unsigned char;
    
    void output_ref(std::ostream& out, const size_t src, const size_t len) {
        out << "(" << src << "," << len << ")";
        if constexpr(verbose) std::cout << "-> (" << src << "," << len << ")" << std::endl;
        if constexpr(m_track_stats) ++m_stats.num_refs;
    }
    
    void output_character(std::ostream& out, const char_t c) {
        out << c;
        if constexpr(verbose) std::cout << "-> " << c << std::endl;
        if constexpr(m_track_stats) ++m_stats.num_literals;
    }
    
    index_t m_window;
    Stats m_stats;
    
public:
    LZ77SlidingWindow(const index_t window) : m_window(window) {
        if(m_window > std::numeric_limits<window_index_t>::max()) {
            throw std::runtime_error("maximum allowed window size exceeded");
        }
    }

    void compress(std::istream& in, std::ostream& out) {
        index_t window_offset = 0;
        
        const auto bufsize = 2 * m_window;
        char_t* buffer = new char_t[bufsize + 1];
        char_t* prev_buffer = m_allow_ext_match ? new char_t[m_window + 1] : nullptr;
        
        SlidingWindowTrie sw_tries[2] = { SlidingWindowTrie(bufsize + 1, 2 * m_window), SlidingWindowTrie(bufsize + 1, 2 * m_window) };
        SlidingWindowTrie* left_trie;
        SlidingWindowTrie* right_trie;
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
        
        bool   ext_match = false; // are we currently matching an extended pattern?
        size_t ext_src = 0;        // the source position of the current extended match
        size_t ext_len = 0;        // the current extended match length
        
        // read initial 2w characters
        if(in) {
            left_trie = &sw_tries[0]; // empty
            in.read((char*)buffer, bufsize);
            const auto r = in.gcount();
            if(r > 0) {
                buffer[r] = (char_t)0; // terminate
                right_trie = &sw_tries[1];
                cur_right_trie = 1;
                right_trie->build(buffer, r, m_window, sa_buffer, lcp_buffer, work_buffer);
                if constexpr(m_track_stats) m_stats.trie_size = std::max(m_stats.trie_size, right_trie->size());
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
                prev_window_start = window_start;
                window_start = b * m_window;
                if constexpr(verbose) std::cout << "entering block " << b << std::endl;
                
                // right trie now becomes left trie
                left_trie = right_trie;
                cur_right_trie = !cur_right_trie;
                right_trie = &sw_tries[cur_right_trie];
                
                // if in extended match, copy first w characters to prev buffer
                if(ext_match) {
                    for(size_t i = 0; i < m_window; i++) {
                        prev_buffer[i] = buffer[i];
                    }
                }
                
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
                    right_trie->build(buffer, last_block_len + r, m_window, sa_buffer, lcp_buffer, work_buffer);
                    if constexpr(m_track_stats) m_stats.trie_size = std::max(m_stats.trie_size, right_trie->size());
                    n += r;
                    last_block_len = r;
                    
                    // if constexpr(verbose) left_trie->print();
                    // if constexpr(verbose) right_trie->print();
                }
            }

            // continue extended match
            if constexpr(m_allow_ext_match) {
                if(ext_match) {
                    if constexpr(verbose) std::cout << "\tcontinuing extended match" << std::endl;
                    
                    const auto c = buffer[i - window_start];
                    const size_t j = ext_src + ext_len;
                    assert(j < i);
                    assert(j >= prev_window_start);
                    const auto x = (j >= window_start) ? buffer[j - window_start] : prev_buffer[j - prev_window_start];
                    
                    if constexpr(verbose) std::cout << "\tread \"" << c << "\", compare against \"" << x << "\"" << std::endl;
                    if(c == x) {
                        if constexpr(verbose) std::cout << "\tcontinuing extended match" << std::endl;
                        ++ext_len;
                        ++i;
                        continue;
                    } else {
                        if constexpr(verbose) std::cout << "\tconcluding extended match" << std::endl;
                        output_ref(out, ext_src, ext_len);
                        ext_match = false;
                    }
                }
            }
            
            // perform match starting at position i
            {
                auto lv = left_trie->cursor();
                bool lsearch = true;
                auto rv = right_trie->cursor();
                bool rsearch = true;

                size_t j = i;
                assert(j < n);
                char_t c;
                while(j < n && (lsearch || rsearch)) {
                    // get next character
                    c = buffer[j - window_start];
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
                
                if constexpr(verbose) std::cout << "search stopped at d(T)=" << lv.depth << " and d(T')=" << rv.depth << std::endl;
                
                if(m_allow_ext_match && j < n && ((lv.reached_leaf() && lv.depth > 1) || (rv.reached_leaf() && rv.depth > 1))) {
                    // we reached a leaf - initiate extended match
                    ext_match = true;
                    ext_src = (lv.depth > rv.depth) ? (prev_window_start + lv.max_pos()) : (window_start + rv.min_pos());
                    ext_len = std::max(lv.depth, rv.depth);
                    if constexpr(verbose) std::cout << "reached a leaf - initiate extended match starting from " << ext_src << std::endl;
                    
                    i += ext_len;
                } else {
                    // conclude match
                    const auto flen = std::max(lv.depth, rv.depth);
                    if(flen > 0) {
                        if(flen > 1) {
                            const auto fsrc = (lv.depth > rv.depth) ? (prev_window_start + lv.max_pos()) : (window_start + rv.min_pos());
                            output_ref(out, fsrc, flen);
                        } else {
                            output_character(out, (lv.depth > 0) ? lv.character() : rv.character());
                        }
                        i += flen;
                    } else {
                        // print unmatched character
                        output_character(out, c);
                        ++i;
                    }
                }
            }
        }
        
        if(ext_match) {
            // stream ended during extended match, print reference
            output_ref(out, ext_src, ext_len);
        }
        
        // clean up
        delete[] sa_buffer;
        delete[] lcp_buffer;
        delete[] work_buffer;
        delete[] buffer;
        if constexpr(m_allow_ext_match) delete[] prev_buffer;
        
        // stats
        if constexpr(m_track_stats) m_stats.input_size = n;
    }

    const Stats& stats() const { return m_stats; }
};

}}} // namespace tdc::comp::lz77
