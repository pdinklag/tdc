#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

#include <tdc/comp/lz77/noop.hpp>
#include <tdc/comp/lz77/lz77_sa.hpp>
#include <tdc/comp/lz77/lz77_sw.hpp>
#include <tdc/comp/lz77/lzfp.hpp>
#include <tdc/comp/lz77/lzqgram.hpp>
//~ #include <tdc/comp/lz77/archive/lzqgram_btree.hpp>
//~ #include <tdc/comp/lz77/archive/lzqgram_hash.hpp>
#include <tdc/comp/lz77/lzqgram_sketch.hpp>
//~ #include <tdc/comp/lz77/archive/lzqgram_sketch2.hpp>
//~ #include <tdc/comp/lz77/archive/lzqgram_sketch3.hpp>
#include <tdc/comp/lz77/lzqgram_trie.hpp>

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
    size_t threshold = 2;
    size_t q = 0;
    size_t window = 1_Ki;
    size_t tau_min = 5;
    size_t tau_max = 5;
    size_t filter_size = 1_Ki;
    size_t cm_width = 1_Ki;
    size_t cm_height = 4;
    
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
        Stats stats;
        {
            auto c = ctor();
            // c.compress(input, std::cout);
            c.compress(input, devnull);
            stats = c.stats();
        }
        
        auto guard = phase.suppress();
        if(group == "sliding") {
            phase.log("window", options.window);
        }
        if(group == "fp") {
            phase.log("tau_min", options.tau_min);
            phase.log("tau_max", options.tau_max);
        }
        if(group == "sketch") {
            phase.log("filter", options.filter_size);
            phase.log("cm_width", options.cm_width);
            phase.log("cm_height", options.cm_height);
            phase.log("num_swaps", stats.num_swaps);
            phase.log("num_extensions", stats.num_extensions);
            phase.log("extension_sum", stats.extension_sum);
        }
        phase.log("input_size", stats.input_size);
        phase.log("num_literals", stats.num_literals);
        phase.log("num_refs", stats.num_refs);
        phase.log("num_factors", stats.num_literals + stats.num_refs);
        phase.log("trie_size", stats.trie_size);
        // std::cout << std::endl;
        std::cout << "RESULT algo=" << name << " group=" << group << " input=" << options.filename << " threshold=" << options.threshold << " " << phase.to_keyval() << std::endl;
    }
}

int main(int argc, char** argv) {
    tlx::CmdlineParser cp;
    cp.add_param_string("file", options.filename, "The input file.");
    cp.add_size_t('t', "threshold", options.threshold, "Minimum reference length (default: 2).");
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
    
    //~ bench("base", "Noop", [](){ return Noop<true>(); });
    bench("base", "SA", [](){ return LZ77SA<true>(options.threshold); });
    
    bench("sliding", "Sliding", [](){ return LZ77SlidingWindow<false, 0, true>(options.window); });
    bench("fp", "FP", [](){ return LZFingerprinting(options.tau_min, options.tau_max); });
    //~ bench("sliding", "SlidingLong(2,8)", [](){ return LZ77SlidingWindow<false, 8, true>(options.window); });
    //~ bench("sliding", "SlidingLong(2,16)", [](){ return LZ77SlidingWindow<false, 16, true>(options.window); });
    //~ bench("sliding", "SlidingLong(2,32)", [](){ return LZ77SlidingWindow<false, 32, true>(options.window); });
    //~ bench("sliding", "SlidingLong(2,64)", [](){ return LZ77SlidingWindow<false, 64, true>(options.window); });
    //~ bench("sliding", "Sliding*", [](){ return LZ77SlidingWindow<true, true>(options.window); });
    
    if(options.q == 0 || options.q == 8) {
        bench("qgram", "Trie(q=8)", [](){ return LZQGram<uint64_t, qgram::TrieProcessor<unsigned char, true>>([](){ return qgram::TrieProcessor<unsigned char, true>(); }, options.threshold); });
        bench("sketch", "Sketch(q=8)", [](){ return LZQGram<uint64_t, qgram::SketchProcessor<uint64_t>>([](){ return qgram::SketchProcessor<uint64_t>(options.filter_size, options.cm_width, options.cm_height); }, options.threshold); });
    }
    
    if(options.q == 0 || options.q == 16) {
        bench("qgram", "Trie(q=16)", [](){ return LZQGram<uint128_t, qgram::TrieProcessor<unsigned char, true>>([](){ return qgram::TrieProcessor<unsigned char, true>(); }, options.threshold); });
        bench("sketch", "Sketch(q=16)", [](){ return LZQGram<uint128_t, qgram::SketchProcessor<uint128_t>>([](){ return qgram::SketchProcessor<uint128_t>(options.filter_size, options.cm_width, options.cm_height); }, options.threshold); });
    }
}
