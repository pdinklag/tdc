#pragma once

#include <iostream>

#include <tlx/container/ring_buffer.hpp>
#include <robin_hood.h>

#include <tdc/io/buffered_reader.hpp>
#include <tdc/hash/rolling.hpp>
#include <tdc/util/augmented_sketch.hpp>
#include <tdc/util/char.hpp>
#include <tdc/util/index.hpp>
#include <tdc/util/literals.hpp>

namespace tdc {
namespace comp {
namespace lz77 {

// a marriage between LZSketch and LZFingerprinting
class LZFingerprintingTop {
    static constexpr bool verbose_ = true;
    
    using RollingFP = hash::RollingKarpRabinFingerprint;
    using RingBuffer = tlx::RingBuffer<char_t>;
    using Sketch = AugmentedSketch<uint64_t, index_t>;

    struct Layer {
        index_t   tau_exp;
        index_t   tau;
        RollingFP roller;
        uint64_t  fp;
        Sketch    sketch;
    };

    index_t tau_min_, tau_max_;
    size_t min_filter_size_, max_filter_size_;
    size_t cm_width_, cm_height_;

    index_t pos_;
    index_t next_factor_;

    RingBuffer window_;
    std::vector<Layer> layers_;

    size_t window_size() const {
        return tau_max_;
    }

    inline void prepare(const char_t* window) {
        // prepare initial window and fingerprints
        assert(pos_ == 0);
        
        for(size_t i = 0; i < window_size(); i++) {
            const char_t c = window[i];
            for(auto& layer : layers_) {
                if(i < layer.tau) layer.fp = layer.roller.roll(layer.fp, 0, c);
            }
            window_.push_back(c);
        }
    }

    template<typename FactorOutput>
    inline void process(const char_t c, FactorOutput& out, const size_t max_tau) {
        assert(window_.size() == window_.max_size()); // window must have been prepared!

        // possibly emit popped literal
        const char_t pop = window_.front();
        
        if(pos_ >= next_factor_) {
            out.emplace_back(pop);
            ++next_factor_;
        }

        // advance sliding window
        const auto prev_pos = pos_;
        window_.pop_front();
        window_.push_back(c);
        ++pos_;

        // process layers - longest first
        index_t layer_mask = tau_max_ - 1;
        for(auto& layer : layers_) {
            if(layer.tau <= max_tau) {
                // if we reached a boundary for this layer, store fingerprint
                if((prev_pos & layer_mask) == 0) {     // prev_pos % layer.tau, but layer.tau is a power of two
                    layer.sketch.count(layer.fp, prev_pos); // since prev_pos is a multiple of layer.tau, we could store a quotient here and compress it, but the used hashtable doesn't support that
                }
                
                // update fingerprint with new character
                const char_t layer_push = window_[layer.tau - 1];
                layer.fp = layer.roller.roll(layer.fp, pop, layer_push);

                // test if fingerprint has been seen before
                if(pos_ >= next_factor_) {
                    index_t prev_occ;
                    if(layer.sketch.is_frequent(layer.fp, prev_occ)) {
                        // seen before - emit reference
                        out.emplace_back(prev_occ, layer.tau);
                        next_factor_ += layer.tau;
                    }
                }
            }
            
            // advance layer
            layer_mask >>= 1;
        }
    }

public:
    inline LZFingerprintingTop(
        const size_t min_filter_size,
        const size_t max_filter_size,
        const size_t cm_width,
        const size_t cm_height,
        const index_t tau_exp_min,
        const index_t tau_exp_max)
        : min_filter_size_(min_filter_size),
          max_filter_size_(max_filter_size),
          cm_width_(cm_width),
          cm_height_(cm_height),
          tau_min_(1ULL << tau_exp_min),
          tau_max_(1ULL << tau_exp_max),
          window_(tau_max_) {

        const size_t num_layers = (tau_exp_max >= tau_exp_min) ? tau_exp_max - tau_exp_min + 1 : 0;
        assert(num_layers > 0);
        layers_.reserve(num_layers);

        size_t filter_shr = num_layers;
        index_t tau_exp = tau_exp_max;
        for(size_t i = 0; i < num_layers; i++) {
            const index_t tau = 1ULL << tau_exp;

            const size_t filter_size = std::max(min_filter_size, max_filter_size >> filter_shr);
            layers_.emplace_back(Layer{ tau_exp, tau, RollingFP(tau), 0, Sketch(filter_size, cm_width, cm_height) });
            --tau_exp;
            --filter_shr;
        }
    }

    template<typename FactorOutput>
    inline void compress(std::istream& in, FactorOutput& out) {
        // init
        next_factor_ = 0;
        pos_ = 0;

        const auto w = window_size();

        // read
        {
            io::BufferedReader<char_t> reader(in, 1_Mi);

            // prepare initial window
            {
                char_t* initial = new char_t[w];
                if(reader.read(initial, w) == w) {
                    prepare(initial);
                }
                delete[] initial;
            }

            // read file
            while(reader) {
                process(reader.read(), out, w);
            }

            // process remainder
            {
                size_t remain = w;
                while(remain) {
                    process(0, out, remain - 1);
                    --remain;
                }
            }
        }
    }
    
    template<typename StatLogger>
    void log_stats(StatLogger& logger) {
        logger.log("tau_min", tau_min_);
        logger.log("tau_max", tau_max_);
        logger.log("filter_min", min_filter_size_);
        logger.log("filter_max", max_filter_size_);
        logger.log("cm_width", cm_width_);
        logger.log("cm_height", cm_height_);
    }
};

}}} // namespace tdc::comp::lz77
