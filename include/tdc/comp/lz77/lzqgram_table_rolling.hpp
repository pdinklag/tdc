#pragma once

#include <bit>
#include <cstdint>
#include <concepts>
#include <iostream>
#include <limits>
#include <stdexcept>

#include <robin_hood.h>
#include <tlx/container/ring_buffer.hpp>

#include <tdc/hash/function.hpp>
#include <tdc/hash/rolling.hpp>
#include <tdc/math/prime.hpp>
#include <tdc/random/permutation.hpp>
#include <tdc/util/index.hpp>

#include "stats.hpp"

namespace tdc {
namespace comp {
namespace lz77 {

template<std::unsigned_integral char_t = unsigned char, bool m_track_stats = false>
class LZQGramTableRolling {
private:
    using buffer_t = tlx::RingBuffer<char_t>;
    using count_t = index_t;

    struct Entry {
        uint64_t last;
        index_t seen_at;
        
        Entry() : last(0), seen_at(0) {
        }
    } __attribute__((__packed__));
    
    size_t m_window;
    size_t m_num_rows, m_num_cols;
    size_t m_row_mask, m_col_mask;
    
    std::vector<std::vector<Entry>> m_data;
    std::vector<hash::RollingKarpRabinFingerprint<char_t>> m_buffers;
    std::vector<hash::Multiplicative> m_hash;
    size_t m_cur_row;
    
    Stats m_stats;
    
    size_t m_pos;
    size_t m_next_factor;
    
    size_t m_last_ref_src;
    size_t m_last_ref_len;
    size_t m_last_ref_src_end;
    
    size_t m_threshold;
    
    size_t hash(const size_t i, const uint64_t key) {
        size_t h = m_hash[i](key);
        h += (h >> 48);
        // std::cout << "\t\th" << std::dec << i << "(0x" << std::hex << key << ") = 0x" << h << std::endl;
        return h & m_col_mask;
    }

    void reset() {
        m_pos = 0;
        m_next_factor = 0;
        
        // TODO table, buffers
    }
    
    void process(char_t c, std::ostream& out) {
        // advance fingerprints
        const char_t front = m_buffers[0].advance(c);
        for(size_t i = 1; i < m_window - m_threshold; i++) {
            m_buffers[i].advance(c);
        }
        
        // test if at least m_window characters have been read
        if(m_buffers[0].full()) {
            // std::cout << "T[" << std::dec << m_pos << "] = " << c << " = 0x" << std::hex << size_t(c) << std::endl;
            
            // process prefixes
            size_t len = m_window;
            for(size_t j = 0; j < m_window - m_threshold; j++) {
                const auto prefix_fp = m_buffers[j].fingerprint();
                // std::cout << "\tprefix " << m_buffers[j].str() << ", fp=0x" << std::hex << prefix_fp << std::endl;
                
                // test if prefix has been seen before
                if(m_pos >= m_next_factor) {
                    for(size_t i = 0; i < m_num_rows; i++) {
                        const auto h = hash(i, prefix_fp);
                        if(m_data[i][h].last == prefix_fp) {
                            const auto src = m_data[i][h].seen_at;
                            
                            // std::cout << "\t\toutput reference (" << std::dec << m_data[i][h].seen_at << "," << len << ")" << std::endl;
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
                        }
                    }
                }
                
                // enter prefix
                {
                    const auto h = hash(m_cur_row, prefix_fp);
                    
                    if constexpr(m_track_stats) {
                        if(m_data[m_cur_row][h].last) ++m_stats.num_collisions;
                    }
                    
                    m_data[m_cur_row][h].last = prefix_fp;
                    m_data[m_cur_row][h].seen_at = m_pos;
                    // std::cout << "\t\tenter into cell [" << std::dec << m_cur_row << "," << h << "]" << std::endl;
                    m_cur_row = (m_cur_row + 1) & m_row_mask;
                }
                
                --len;
            }
            
            // advance
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
            
            // advance position
            ++m_pos;
        }
    }

public:
    LZQGramTableRolling(size_t window, size_t num_cols, size_t num_rows, size_t threshold = 2, uint64_t seed = random::DEFAULT_SEED)
        : m_window(window),
          m_num_cols(num_cols),
          m_num_rows(num_rows),
          m_col_mask(num_cols-1),
          m_row_mask(num_rows-1),
          m_threshold(threshold),
          m_cur_row(0),
          m_last_ref_src(SIZE_MAX) {

        if(!std::has_single_bit(num_cols)) throw std::runtime_error("use powers of two for num_cols");
        if(!std::has_single_bit(num_rows)) throw std::runtime_error("use powers of two for num_rows");

        // initialize data rows and prime offsets
        assert(m_num_rows <= math::NUM_POOL_PRIMES);
        random::Permutation perm(math::NUM_POOL_PRIMES, seed);
        
        m_hash.reserve(m_num_rows);
        m_data.reserve(m_num_rows);
        
        for(size_t i = 0; i < m_num_rows; i++) {
            m_hash.emplace_back(hash::Multiplicative(math::PRIME_POOL[perm(i)]));
            m_data.emplace_back(std::vector<Entry>(m_num_cols, Entry()));
            // std::cout << "h" << std::dec << " = 0x" << std::hex << math::PRIME_POOL[perm(i)] << std::endl;
        }
        
        // initialize fingerprints
        {
            size_t prefix_len = m_window;
            m_buffers.reserve(m_window - m_threshold);
            for(size_t i = 0; i < m_window - m_threshold; i++) {
                m_buffers.emplace_back(hash::RollingKarpRabinFingerprint<char_t>(prefix_len));
                --prefix_len;
            }
        }
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
        for(size_t i = 0; i < m_window - 1; i++) {
            process(0, out);
        }
        
        if(m_last_ref_src != SIZE_MAX) {
            out << "(" << m_last_ref_src << "," << m_last_ref_len << ")";
            if constexpr(m_track_stats) ++m_stats.num_refs;
        }
        
        if constexpr(m_track_stats) m_stats.input_size = m_pos;
        // if constexpr(m_track_stats) m_stats.debug = m_filter.size();
    }
    
    const Stats& stats() const { return m_stats; }
};
    
}}} // namespace tdc::comp::lz77