#pragma once

#include <cstdint>
#include <concepts>
#include <iostream>
#include <limits>

#include <robin_hood.h>
#include <tlx/container/ring_buffer.hpp>

#include <tdc/hash/function.hpp>
#include <tdc/math/prime.hpp>
#include <tdc/random/permutation.hpp>
#include <tdc/util/index.hpp>

#include "stats.hpp"

namespace tdc {
namespace comp {
namespace lz77 {

template<std::unsigned_integral char_t = unsigned char, std::unsigned_integral packed_chars_t = uint64_t, bool m_track_stats = false>
class LZQGramTable {
private:
    using buffer_t = tlx::RingBuffer<char_t>;
    using count_t = index_t;

    static constexpr size_t pack_num = sizeof(packed_chars_t) / sizeof(char_t);
    static packed_chars_t pack(const buffer_t& buffer) {
        packed_chars_t packed = buffer[0];
        for(size_t i = 1; i < pack_num; i++) {
            packed <<= std::numeric_limits<char_t>::digits;
            packed |= buffer[i];
        }
        return packed;
    }
    
    struct Entry {
        packed_chars_t last;
        index_t seen_at;
        
        Entry() : last(0), seen_at(0) {
        }
    } __attribute__((__packed__));
    
    size_t m_num_rows, m_num_cols;
    std::vector<hash::Multiplicative> m_hash;
    std::vector<std::vector<Entry>> m_data;
    size_t m_cur_row;
    
    size_t hash(const size_t i, const packed_chars_t& key) const {
        size_t h = m_hash[i](key);
        h += (h >> 48);
        // std::cout << "\t\th" << std::dec << i << "(0x" << std::hex << key << ") = 0x" << h << std::endl;
        return h % m_num_cols;
    }
    
    buffer_t m_buffer;
    std::unique_ptr<Stats> m_stats;
    
    size_t m_pos;
    size_t m_next_factor;
    
    size_t m_threshold;

    void reset() {
        m_pos = 0;
        m_next_factor = 0;
        
        // TODO table
        
        m_buffer.clear();
        
        if constexpr(m_track_stats) {
            m_stats = std::make_unique<Stats>();
        }
    }
    
    void process(char_t c, std::ostream& out) {
        // std::cout << "T[" << std::dec << m_pos << "] = " << c << " = 0x" << std::hex << size_t(c) << std::endl;
        m_buffer.push_back(c);
        
        if(m_buffer.size() == pack_num) {
            // read buffer contents
            // std::cout << "process buffer" << std::endl;
            auto prefix = pack(m_buffer);
            size_t len = pack_num;
            for(size_t i = 0; i < pack_num - (m_threshold-1); i++) {
                // std::cout << "\tprefix 0x" << std::hex << prefix << std::endl;
                
                // test if prefix has been seen before
                size_t cur_h;
                if(m_pos >= m_next_factor) {
                    for(size_t i = 0; i < m_num_rows; i++) {
                        const auto h = hash(i, prefix);
                        if(m_data[i][h].last == prefix) {
                            // std::cout << "\t\toutput reference (" << std::dec << m_data[i][h].seen_at << "," << len << ")" << std::endl;
                            out << "(" << m_data[i][h].seen_at << "," << len << ")";
                            m_next_factor = m_pos + len;
                            
                            if constexpr(m_track_stats) ++m_stats->num_refs;
                            break;
                        }
                    }
                }
                
                // enter
                {
                    const auto h = hash(m_cur_row, prefix);
                    m_data[m_cur_row][h].last = prefix;
                    m_data[m_cur_row][h].seen_at = m_pos;
                    // std::cout << "\t\tenter into cell [" << std::dec << m_cur_row << "," << h << "]" << std::endl;
                    m_cur_row = (m_cur_row + 1) % m_num_rows;
                }
                
                prefix >>= std::numeric_limits<char_t>::digits;
                --len;
            }
            
            // advance buffer
            if(m_pos >= m_next_factor) {
                // output literal
                // std::cout << "\toutput literal " << m_buffer.front() << std::endl;
                out << m_buffer.front();
                ++m_next_factor;
                
                if constexpr(m_track_stats) ++m_stats->num_literals;
            }
            m_buffer.pop_front();
            
            // advance
            ++m_pos;
        }
    }

public:
    LZQGramTable(size_t num_cols, size_t num_rows, size_t threshold = 2, uint64_t seed = random::DEFAULT_SEED) : m_buffer(pack_num), m_num_cols(num_cols), m_num_rows(num_rows), m_threshold(threshold), m_cur_row(0) {
        // initialize hash functions and data rows
        assert(m_num_rows <= math::NUM_POOL_PRIMES);
        random::Permutation perm(math::NUM_POOL_PRIMES, seed);
        
        m_hash.reserve(m_num_rows);
        m_data.reserve(m_num_rows);
        
        for(size_t i = 0; i < m_num_rows; i++) {
            m_hash.emplace_back(hash::Multiplicative(math::PRIME_POOL[perm(i)]));
            m_data.emplace_back(std::vector<Entry>(m_num_cols, Entry()));
            // std::cout << "h" << std::dec << " = 0x" << std::hex << math::PRIME_POOL[perm(i)] << std::endl;
        }
    }

    void compress(std::istream& in, std::ostream& out) {
        reset();
        
        char_t c;
        while(in.read((char*)&c, sizeof(char_t))) {
            process(c, out);
        }
        
        // process remainder
        for(size_t i = 0; i < pack_num - 1; i++) {
            process(0, out);
        }
        
        if constexpr(m_track_stats) m_stats->input_size = m_pos;
        // if constexpr(m_track_stats) m_stats->debug = m_filter.size();
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