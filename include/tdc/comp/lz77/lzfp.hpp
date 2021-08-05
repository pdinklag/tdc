#pragma once

#include <bit>
#include <cstdint>
#include <concepts>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>

#include <tlx/container/ring_buffer.hpp>
#include <robin_hood.h>

#include <tdc/io/buffered_reader.hpp>
#include <tdc/hash/rolling.hpp>
#include <tdc/util/char.hpp>
#include <tdc/util/index.hpp>
#include <tdc/util/literals.hpp>

namespace tdc {
namespace comp {
namespace lz77 {

class LZFingerprinting {
private:
    using RollingFP = hash::RollingKarpRabinFingerprint;
    using RefMap = robin_hood::unordered_map<uint64_t, index_t>;
    using RingBuffer = tlx::RingBuffer<char_t>;

    index_t m_tau_min, m_tau_max;

    index_t m_pos;
    index_t m_next_factor;

    index_t m_num_layers;
    index_t m_min_mask;

    struct Layer {
        index_t   tau_exp;
        index_t   tau;
        RollingFP roller;
        uint64_t  fp;
        RefMap    refs;
    };

    RingBuffer m_window;
    std::vector<Layer> m_layers;

    template<typename FactorOutput>
    inline void process(const char_t c, FactorOutput& out) {
        // update
        if(m_num_layers > 0) {
            index_t mask = m_layers[0].tau - 1;
            
            const size_t w = m_window.size();
            for(size_t i = 0; i < m_num_layers; i++) {
                auto& layer = m_layers[i];
                if(m_pos > 0 && (m_pos & mask) == 0) {
                    // reached block boundary on layer i, store current fingerprint
                    layer.refs[layer.fp] = (m_pos - 1) >> layer.tau_exp;
                }

                // advance rolling fingerprint
                const char_t out_c = (w >= layer.tau) ? m_window[w - layer.tau] : 0;
                layer.fp = layer.roller.roll(layer.fp, out_c, c);

                // roll mask
                mask >>= 1ULL;
            }
        }

        // advance window
        char_t out_c = 0;
        if(m_window.size() == m_window.max_size()) {
            out_c = m_window.front();
            m_window.pop_front();
        }
        m_window.push_back(c);

        //
        bool emit = (m_pos >= m_window.max_size()) && (m_pos - m_window.max_size() >= m_next_factor);

        // process layers greedily - longest tau first
        if(emit) {
            for(size_t i = 0; i < m_num_layers; i++) {
                const auto& layer = m_layers[i];

                // test for each layer if current fingerprint was seen before (greedy, i.e., longest tau first)
                auto it = layer.refs.find(layer.fp);
                if(it != layer.refs.end()) {
                    // output shifted literal
                    out.emplace_back(out_c);
                    
                    // output reference
                    const size_t fsrc = it->second << layer.tau_exp;
                    const size_t flen = layer.tau;
                    
                    m_next_factor += flen + 1; // also count emitted literal
                    out.emplace_back(fsrc, flen);

                    emit = false;
                    break; // don't consider shorter layers
                }
            }
        }

        if(emit) {
            // output literal
            ++m_next_factor;
            out.emplace_back(out_c);
        }
        
        // advance
        ++m_pos;
    }

public:
    inline LZFingerprinting(const index_t tau_exp_min, const index_t tau_exp_max)
        : m_tau_min(1ULL << tau_exp_min),
          m_tau_max(1ULL << tau_exp_max),
          m_window(1ULL << tau_exp_max) {
        m_num_layers = (tau_exp_max >= tau_exp_min) ? tau_exp_max - tau_exp_min + 1 : 0;

        index_t tau_exp = tau_exp_max;
        for(size_t i = 0; i < m_num_layers; i++) {
            const index_t tau = 1ULL << tau_exp;
            m_layers.emplace_back(Layer{ tau_exp, tau, RollingFP(tau), 0, RefMap() });
            --tau_exp;
        }
    }

    template<typename FactorOutput>
    inline void compress(std::istream& in, FactorOutput& out) {
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

        // output remaining literals
        while(m_window.size() > 0) {
            const bool emit = (m_pos >= m_window.max_size()) && (m_pos - m_window.max_size() >= m_next_factor);
            if(emit) {
                ++m_next_factor;
                out.emplace_back(m_window.front());
            }

            m_window.pop_front();
            ++m_pos;
        }
    }
    
    template<typename StatLogger>
    void log_stats(StatLogger& logger) {
        logger.log("tau_min", m_tau_min);
        logger.log("tau_max", m_tau_max);
    }
};

}}} // namespace tdc::comp::lz77
