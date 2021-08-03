#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

#include <tdc/comp/lz77/factor_buffer.hpp>

#include <tdc/comp/lz77/noop.hpp>
#include <tdc/comp/lz77/lz77_sa.hpp>
#include <tdc/comp/lz77/lz77_sw.hpp>
#include <tdc/comp/lz77/lzfp.hpp>
#include <tdc/comp/lz77/lzsketch.hpp>

#include <tdc/uint/uint128.hpp>
#include <tdc/uint/uint256.hpp>
#include <tdc/io/null_ostream.hpp>
#include <tdc/stat/phase.hpp>
#include <tdc/util/literals.hpp>

#include <tlx/cmdline_parser.hpp>

using namespace tdc::comp::lz77;

using char_t = unsigned char;

struct {
    std::string filename;
    size_t q = 0;
    size_t window = 1_Ki;
    size_t tau_min = 5;
    size_t tau_max = 5;
    size_t filter_size = 1_Ki;
    size_t cm_width = 1_Ki;
    size_t cm_height = 2;
    
    std::string ds;
    
    bool do_bench(const std::string& group) {
        return ds.length() == 0 || ds == group;
    }
} options;

template<typename ctor_t>
void bench(const std::string& group, std::string&& name, ctor_t ctor) {
    if(!options.do_bench(group)) return;
    
    std::ifstream input(options.filename);
    tdc::io::NullOStream devnull;
    
    {
        tdc::stat::Phase phase("compress");
        FactorBuffer factors;

        {
            auto c = ctor();
            c.compress(input, factors);        
            
            auto guard = phase.suppress();
            c.log_stats(phase);
        }

        auto guard = phase.suppress();
        auto stats = factors.gather_stats();

        phase.log("input_size", stats.input_size);
        phase.log("num_literals", stats.num_literals);
        phase.log("num_refs", stats.num_refs);
        phase.log("num_factors", factors.size());
        phase.log("min_ref_len", stats.min_ref_len);
        phase.log("max_ref_len", stats.max_ref_len);
        phase.log("avg_ref_len", std::lround(stats.avg_ref_len));
        phase.log("min_ref_dist", stats.min_ref_dist);
        phase.log("max_ref_dist", stats.max_ref_dist);
        phase.log("avg_ref_dist", std::lround(stats.avg_ref_dist));
        
        std::cout << "RESULT algo=" << name << " group=" << group << " input=" << options.filename << " " << phase.to_keyval() << std::endl;
    }
}

template<typename ctor_a, typename ctor_b>
class Merge {
private:
    ctor_a construct_a_;
    ctor_b construct_b_;

public:
    inline Merge(ctor_a construct_a, ctor_b construct_b) : construct_a_(construct_a), construct_b_(construct_b) {
    }

    inline void compress(std::istream& in, FactorBuffer& out) {
        FactorBuffer factors_a, factors_b;
        
        // first compress using A
        {
            auto a = construct_a_();
            a.compress(in, factors_a);
        }

        // rewind
        in.clear();
        in.seekg(0);

        // compress using B
        {
            auto b = construct_b_();
            b.compress(in, factors_b);
        }

        // merge factors
        out = FactorBuffer::merge(factors_a, factors_b);
    }

    template<typename StatLogger>
    void log_stats(StatLogger& logger) {
    }
};

int main(int argc, char** argv) {
    tlx::CmdlineParser cp;
    cp.add_param_string("file", options.filename, "The input file.");
    cp.add_string('a', "group", options.ds, "The algorithm group to benchmark.");
    cp.add_size_t('q', "min-qgram", options.q, "The minimum q-gram length.");
    cp.add_bytes('w', "window", options.window, "The window length for sliding window algorithms (default: 1024).");
    cp.add_bytes("tau-min", options.tau_min, "The minimum window length exponent for fingerprinting algorithms (default: 5, pertaining to 32).");
    cp.add_bytes("tau-max", options.tau_max, "The maximum window length exponent for fingerprinting algorithms (default: 5, pertaining to 32).");
    cp.add_bytes("filter", options.filter_size, "The size of the sketch filter if used (default: 1024).");
    cp.add_bytes("cm-width", options.cm_width, "The width of the count-min sketch if used (default: 1024).");
    cp.add_bytes("cm-height", options.cm_height, "The height of the count-min sketch if used (default: 4).");
    if(!cp.process(argc, argv)) {
        return -1;
    }

    options.tau_max = std::max(options.tau_min, options.tau_max);

    // constructors
    auto lz77_sa     = [](){ return LZ77SA(); };
    auto lz77_sw     = [](){ return LZ77SlidingWindow<false>(options.window); };
    auto lz77_sw_ext = [](){ return LZ77SlidingWindow<true>(options.window); };
    auto lz_fp       = [](){ return LZFingerprinting(options.tau_min, options.tau_max); };
    auto lz_sketch8  = [](){ return LZSketch<uint64_t>(options.filter_size, options.cm_width, options.cm_height); };
    auto lz_sketch16 = [](){ return LZSketch<uint128_t>(options.filter_size, options.cm_width, options.cm_height); };
    auto lz_sketch32 = [](){ return LZSketch<uint256_t>(options.filter_size, options.cm_width, options.cm_height); };
    auto merge_fp_sw = [&](){ return Merge(lz_fp, lz77_sw); };
    auto merge_sw_fp = [&](){ return Merge(lz77_sw, lz_fp); };
    
    //~ bench("base", "Noop", [](){ return Noop(); });
    bench("base", "SA", lz77_sa);
    
    bench("sliding", "Sliding", lz77_sw);
    //~ bench("sliding", "Sliding*", lz77_sw_ext);
    
    bench("fp", "FP", lz_fp);
    bench("merge", "Merge(Sliding, FP)", merge_sw_fp);

    if(options.q == 0 || options.q == 8) {
        bench("sketch", "Sketch(q=8)", lz_sketch8);

        auto merge_fp_sketch8 = [&](){ return Merge(lz_fp, lz_sketch8); };
        bench("merge", "Merge(Sliding, Sketch(q=8))", [&](){ return Merge(lz77_sw, lz_sketch8); });
        bench("merge", "Merge(FP, Sketch(q=8))", merge_fp_sketch8);
        bench("merge", "Merge(Sliding, Merge(FP, Sketch(q=8)))", [&](){ return Merge(lz77_sw, merge_fp_sketch8); });
    }
    
    if(options.q == 0 || options.q == 16) {
        bench("sketch", "Sketch(q=16)", lz_sketch16);
        
        auto merge_fp_sketch16 = [&](){ return Merge(lz_fp, lz_sketch16); };
        bench("merge", "Merge(Sliding, Sketch(q=16))", [&](){ return Merge(lz77_sw, lz_sketch16); });
        bench("merge", "Merge(FP, Sketch(q=16))", merge_fp_sketch16);
        bench("merge", "Merge(Sliding, Merge(FP, Sketch(q=16)))", [&](){ return Merge(lz77_sw, merge_fp_sketch16); });
    }

    if(options.q == 0 || options.q == 32) {
        bench("sketch", "Sketch(q=32)", lz_sketch32);

        auto merge_fp_sketch32 = [&](){ return Merge(lz_fp, lz_sketch32); };
        bench("merge", "Merge(Sliding, Sketch(q=32))", [&](){ return Merge(lz77_sw, lz_sketch32); });
        bench("merge", "Merge(FP, Sketch(q=32))", merge_fp_sketch32);
        bench("merge", "Merge(Sliding, Merge(FP, Sketch(q=32)))", [&](){ return Merge(lz77_sw, merge_fp_sketch32); });
    }
}
