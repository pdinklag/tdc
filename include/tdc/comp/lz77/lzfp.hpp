#pragma once

#include <bit>
#include <cstdint>
#include <concepts>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>

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

private:
    using RollingFP = hash::RollingKarpRabinFingerprint<char_t>;
    using RefMap = robin_hood::unordered_map<uint64_t, index_t>;

    index_t m_pos;
    index_t m_next_factor;

    index_t m_num_layers;
    index_t m_min_mask;

    struct Layer {
        index_t   tau_exp;
        index_t   tau;
        RollingFP rolling;
        RefMap    refs;
    };

    std::vector<Layer> m_layers;
    Stats m_stats;

    inline void process(char_t c, std::ostream& out) {
        // update
        if(m_pos > 0) {
            for(size_t i = 0; i < m_num_layers; i++) {
                auto& layer = m_layers[i];
                if((m_pos & layer.tau) == 0) {
                    // reached block boundary on layer i, store current fingerprint
                    layer.refs[layer.rolling.fingerprint()] = (m_pos - 1) >> layer.tau_exp;
                }

                // update fingerprints
                layer.rolling.advance(c);
                // FIXME: currently each layer stores a window!
            }
        }

        // process layers greedily - longest tau first
        if(m_pos >= m_next_factor) {
            for(size_t i = 0; i < m_num_layers; i++) {
                const auto& layer = m_layers[i];

                // test for each layer if current fingerprint was seen before (greedy, i.e., longest tau first)
                const auto fp = layer.rolling.fingerprint();
                auto it = layer.refs.find(fp);
                if(it != layer.refs.end()) {
                    // output reference
                    if constexpr(track_stats) {
                        ++m_stats.num_refs;
                    }

                    const size_t fsrc = it->second;
                    const size_t flen = layer.tau;
                    
                    m_next_factor = m_pos + flen;
                    out << "(" << fsrc << "," << flen << ")";
                    //~ std::cout << "reference at pos=" << m_pos << " to src=" << fsrc << " and len=" << flen << ", fp=" << fp << std::endl;
                    break; // don't consider shorter layers
                }
            }
        }

        if(m_pos >= m_next_factor) {
            // output literal
            if constexpr(track_stats) {
                ++m_stats.num_literals;
            }
            m_next_factor = m_pos + 1;
            out << c;
        }
        
        // advance
        ++m_pos;
    }

public:
    inline LZFingerprinting(const index_t tau_exp_min, const index_t tau_exp_max) {
        m_num_layers = (tau_exp_max >= tau_exp_min) ? tau_exp_max - tau_exp_min + 1 : 0;

        index_t tau_exp = tau_exp_max;
        for(size_t i = 0; i < m_num_layers; i++) {
            const index_t tau = 1ULL << tau_exp;
            m_layers.emplace_back(Layer{ tau_exp, tau, RollingFP(tau), RefMap() });
            --tau_exp;
        }
    }

    inline void compress(std::istream& in, std::ostream& out) {
        // init
        m_next_factor = 0;
        m_pos = 0;

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
