#pragma once

#include <bit>
#include <cstdint>
#include <concepts>
#include <iostream>
#include <limits>
#include <stdexcept>

#include <tdc/hash/function.hpp>
#include <tdc/math/prime.hpp>
#include <tdc/random/permutation.hpp>
#include <tdc/util/index.hpp>

#include "stats.hpp"

namespace tdc {
namespace comp {
namespace lz77 {

template<size_t m_num_cols, size_t m_num_rows, std::unsigned_integral packed_chars_t = uint64_t, std::unsigned_integral char_t = unsigned char, bool m_track_stats = false>
class LZQGramTable {
private:
    using count_t = index_t;

    static constexpr size_t pack_num = sizeof(packed_chars_t) / sizeof(char_t);
    static constexpr size_t char_bits = std::numeric_limits<char_t>::digits;
    static constexpr uint64_t m_col_prime = math::prime_predecessor(m_num_cols);
    
    static char_t extract_front(packed_chars_t packed) {
        constexpr size_t front_rsh = (pack_num - 1) * std::numeric_limits<char_t>::digits;
        return (char_t)(packed >> front_rsh);
    }
    
    struct Entry {
        packed_chars_t last;
        index_t seen_at;
        
        Entry() : last(0), seen_at(0) {
        }
    } __attribute__((__packed__));

    std::vector<hash::Multiplicative> m_hash;
    std::vector<std::vector<Entry>> m_data;
    
    uint64_t hash(const size_t i, const packed_chars_t& key) const {
        // std::cout << "\t\th" << std::dec << i << "(0x" << std::hex << key << ") = 0x" << h << std::endl;
        return (uint64_t)((key % m_col_prime) % m_num_cols);
    }
    
    packed_chars_t m_buffer;
    size_t m_read;
    
    Stats m_stats;
    
    size_t m_pos;
    size_t m_next_factor;
    
    size_t m_last_ref_src;
    size_t m_last_ref_len;
    size_t m_last_ref_src_end;
    
    size_t m_threshold;

    void reset() {
        m_pos = 0;
        m_next_factor = 0;
        
        // TODO table
        
        m_buffer = 0;
        m_read = 0;
    }
    
    void process(char_t c, std::ostream& out) {
        // std::cout << "T[" << std::dec << m_pos << "] = " << c << " = 0x" << std::hex << size_t(c) << std::endl;
        const char_t front = extract_front(m_buffer);
        m_buffer = (m_buffer << char_bits) | c;
        ++m_read;
        
        if(m_read >= pack_num) {
            // read buffer contents
            // std::cout << "process buffer" << std::endl;
            auto prefix = m_buffer;
            size_t len = pack_num;
            for(size_t j = 0; j < pack_num - (m_threshold-1); j++) {
                // std::cout << "\tprefix 0x" << std::hex << prefix << std::endl;
                
                // test if prefix has been seen before
                size_t cur_h;
                if(m_pos >= m_next_factor) {
                    for(size_t i = 0; i < m_num_rows; i++) {
                        const auto h = hash(i, prefix);
                        if(m_data[i][h].last == prefix) {
                            const auto src = m_data[i][h].seen_at;
                            m_data[i][h].seen_at = m_pos;
                            
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
                        } else {
                            if(m_data[i][h].last) ++m_stats.num_collisions;
                            m_data[i][h].last = prefix;
                            m_data[i][h].seen_at = m_pos;
                        }
                    }
                }
                
                prefix >>= std::numeric_limits<char_t>::digits;
                --len;
            }
            
            // advance buffer
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
            
            // advance
            ++m_pos;
        }
    }

public:
    LZQGramTable(size_t threshold = 2, uint64_t seed = random::DEFAULT_SEED)
        : m_buffer(0),
          m_read(0),
          m_threshold(threshold),
          m_last_ref_src(SIZE_MAX) {

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