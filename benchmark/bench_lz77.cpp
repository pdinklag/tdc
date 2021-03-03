#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

#include <tdc/comp/lz77/noop.hpp>
#include <tdc/comp/lz77/lz77_sa.hpp>
#include <tdc/comp/lz77/lzqgram_hash.hpp>
#include <tdc/comp/lz77/lzqgram_sketch.hpp>

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
} options;

template<typename ctor_t>
void bench(std::string&& name, ctor_t ctor) {
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
        phase.log("input_size", stats.input_size);
        phase.log("num_literals", stats.num_literals);
        phase.log("num_refs", stats.num_refs);
        phase.log("num_collisions", stats.num_collisions);
        phase.log("num_extensions", stats.num_extensions);
        phase.log("extension_sum", stats.extension_sum);
        phase.log("trie_size", stats.trie_size);
        // std::cout << std::endl;
        std::cout << "RESULT algo=" << name << " threshold=" << options.threshold << " " << phase.to_keyval() << std::endl;
    }
}

int main(int argc, char** argv) {
    tlx::CmdlineParser cp;
    cp.add_param_string("file", options.filename, "The input file.");
    cp.add_size_t('t', "threshold", options.threshold, "Minimum reference length.");
    if(!cp.process(argc, argv)) {
        return -1;
    }
    
    bench("Noop()", [](){ return Noop<true>(); });
    bench("LZ77SA()", [](){ return LZ77SA<true>(options.threshold); });
    bench("LZQGramHash(8, 8_Mi)", [](){ return LZQGramHash<8_Mi, uint64_t, char_t, true>(options.threshold); });
    bench("LZQGramSketch(8, 8_Mi)", [](){ return LZQGramSketch<8_Mi, uint64_t, char_t, true>(options.threshold); });
    bench("LZQGramHash(16, 4_Mi)", [](){ return LZQGramHash<4_Mi, uint128_t, char_t, true>(options.threshold); });
    bench("LZQGramSketch(16, 4_Mi)", [](){ return LZQGramSketch<4_Mi, uint128_t, char_t, true>(options.threshold); });
}
