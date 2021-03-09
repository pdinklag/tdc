#pragma once

#include <bit>
#include <cstdint>
#include <concepts>
#include <iostream>
#include <limits>
#include <stdexcept>

#include <tdc/util/index.hpp>

#include "stats.hpp"

namespace tdc {
namespace comp {
namespace lz77 {

template<typename processor_t, std::unsigned_integral qgram_t = uint64_t, std::unsigned_integral m_char_t = unsigned char, std::endian m_qgram_endian = std::endian::little, bool m_track_stats = false>
class LZQGram {
public:
    using char_t = m_char_t;
    static constexpr size_t char_bits = std::numeric_limits<char_t>::digits;
    
    static constexpr size_t q = sizeof(qgram_t) / sizeof(char_t);
    static constexpr std::endian qgram_endian = m_qgram_endian;
    static constexpr bool track_stats = m_track_stats;

private:
    static constexpr size_t REF_INVALID = SIZE_MAX;
    
    qgram_t m_qgram;
    size_t m_read;
    
    Stats m_stats;
    
    size_t m_pos;
    size_t m_next_factor;
    
    size_t m_cur_src;
    size_t m_cur_len;
    
    size_t m_threshold;
    
    bool m_processor_needs_reset;
    processor_t m_processor;
    
    void output_current_ref(std::ostream& out) {
        if(m_cur_src != REF_INVALID) {
            // std::cout << "\toutput reference (" << std::dec << m_cur_src << "," << m_cur_len << ")" << std::endl;
            out << "(" << m_cur_src << "," << m_cur_len << ")";
            if constexpr(m_track_stats) ++m_stats.num_refs;
        }
    }

    void reset() {
        m_pos = 0;
        m_next_factor = 0;
        
        m_qgram = 0;
        m_read = 0;
        
        m_cur_src = REF_INVALID;
        m_cur_len = 0;
        
        if(m_processor_needs_reset) {
            m_processor = processor_t();
        }
    }

    void process(char_t c, std::ostream& out) {
        if constexpr(m_qgram_endian == std::endian::little) {
            m_qgram = (m_qgram << char_bits) | c;
        } else {
            static constexpr size_t lsh = char_bits * (q - 1);
            m_qgram = (m_qgram >> char_bits) | ((qgram_t)c << lsh);
        }
        
        ++m_read;
        if(m_read >= q) {
            // process qgram
            m_processor.process(*this, out);
            
            // test whether to output literal
            if(m_pos >= m_next_factor) {
                // output current reference, if any, and invalidate
                output_current_ref(out);
                m_cur_src = REF_INVALID;
                m_cur_len = 0;
                
                // output literal
                // std::cout << "\toutput literal " << buffer_front() << std::endl;
                out << buffer_front();
                ++m_next_factor;
                
                if constexpr(m_track_stats) ++m_stats.num_literals;
            }
            
            // advance
            ++m_pos;
        }
    }

public:
    LZQGram(size_t threshold = 2)
        : m_qgram(0),
          m_read(0),
          m_threshold(threshold),
          m_cur_src(REF_INVALID),
          m_processor_needs_reset(false) {
    }
    
    size_t pos() const { return m_pos; }
    qgram_t qgram() const { return m_qgram; }
    qgram_t qgram_prefix(const size_t len) const { return m_qgram >> (q - len) * char_bits; }
    size_t threshold() const { return m_threshold; }

    char_t buffer_front() {
        constexpr size_t front_rsh = (q - 1) * char_bits;
        return (char_t)(m_qgram >> front_rsh);
    }
    
    void output_ref(std::ostream& out, const size_t src, const size_t len) {
        if(m_pos >= m_next_factor && len >= m_threshold) {
            if(src == m_cur_src + m_cur_len) {
                // extend current reference
                // std::cout << "\textend reference (" << std::dec << m_cur_src << "," << m_cur_len << ") to length " << (m_cur_len+len) << std::endl;
                
                m_cur_len += len;          
                if constexpr(m_track_stats) {
                    ++m_stats.num_extensions;
                    m_stats.extension_sum += len;
                }
            } else {
                // output current reference
                output_current_ref(out);
                
                // start new reference
                m_cur_src = src;
                m_cur_len = len;
            }
            
            m_next_factor = m_pos + len;
        }
    }

    void compress(std::istream& in, std::ostream& out) {
        reset();
        m_processor_needs_reset = true;
        
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
        for(size_t i = 0; i < q - 1; i++) {
            process(0, out);
        }
        
        // output last reference, if any
        output_current_ref(out);
        
        if constexpr(m_track_stats) m_stats.input_size = m_pos;
    }
    
    const Stats& stats() const { return m_stats; }
    Stats& stats() { return m_stats; }
};

namespace qgram {
    class NoopProcessor {
    public:
        template<typename lzqgram_t>
        void process(lzqgram_t& c, std::ostream& out) {
        }
    };
}
    
}}} // namespace tdc::comp::lz77
