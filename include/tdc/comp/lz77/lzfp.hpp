#pragma once

#include <bit>
#include <cstdint>
#include <concepts>
#include <iostream>
#include <limits>
#include <stdexcept>

#include <robin_hood.h>

#include <tdc/io/buffered_reader.hpp>
#include <tdc/hash/rolling.hpp>
#include <tdc/util/index.hpp>
#include <tdc/util/literals.hpp>

#include "stats.hpp"

namespace tdc {
namespace comp {
namespace lz77 {

class LZFingerprinting {
public:
    using char_t = unsigned char;
    static constexpr bool track_stats = true;

    using RollingFP = hash::RollingKarpRabinFingerprint<char_t>;

private:
    index_t m_pos;
    index_t m_block_start;
    index_t m_tau;

    index_t m_next_factor;
    
    RollingFP m_fp_block;
    RollingFP m_fp_current;
    robin_hood::unordered_map<uint64_t, index_t> m_ref;
    
    Stats m_stats;

    inline void process(char_t c, std::ostream& out) {
        if(m_pos && m_pos % m_tau == 0) {
            // store fingerprint
            m_ref[m_fp_block.fingerprint()] = m_block_start;
            m_block_start = m_pos;
        }

        // update fingerprints
        m_fp_block.advance(c);
        m_fp_current.advance(c);

        // test if current was seen before
        if(m_pos >= m_next_factor)
        {
            const auto fp = m_fp_current.fingerprint();
            auto it = m_ref.find(fp);
            if(it != m_ref.end()) {
                if constexpr(track_stats) {
                    ++m_stats.num_refs;
                }
                m_next_factor = m_pos + m_tau;
                out << "(" << it->second << "," << m_tau << ")";
            } else {
                if constexpr(track_stats) {
                    ++m_stats.num_literals;
                }
                m_next_factor = m_pos + 1;
                out << c;
            }
        }
        
        // advance
        ++m_pos;
    }

public:
    inline LZFingerprinting(const index_t tau) : m_tau(tau) {
        m_fp_block = RollingFP(m_tau);
        m_fp_current = RollingFP(m_tau);
    }

    inline void compress(std::istream& in, std::ostream& out) {
        // init
        m_next_factor = 0;
        m_pos = 0;
        m_block_start = 0;

        // read
        {
            io::BufferedReader<char_t> reader(in, 1_Mi);
            while(reader) {
                process(reader.read(), out);
            }
        }

        if constexpr(track_stats) m_stats.input_size = m_pos;
    }
    
    inline const Stats& stats() const { return m_stats; }
};

}}} // namespace tdc::comp::lz77
