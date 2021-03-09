#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

#include <tdc/comp/lz77/noop.hpp>
#include <tdc/comp/lz77/lz77_sa.hpp>
#include <tdc/comp/lz77/lzqgram.hpp>
#include <tdc/comp/lz77/lzqgram_hash.hpp>
#include <tdc/comp/lz77/lzqgram_sketch.hpp>
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
        phase.log("num_swaps", stats.num_swaps);
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
    bench("LZQGram<8, Noop>", [](){ return LZQGram<qgram::NoopProcessor, uint64_t, char_t, true>(options.threshold); });
    bench("LZQGram<8, Trie<UL>>", [](){ return LZQGram<qgram::TrieProcessor<TrieList<char_t>>, uint64_t, char_t, true>(options.threshold); });
    bench("LZQGram<8, Trie<HT>>", [](){ return LZQGram<qgram::TrieProcessor<TrieHash<char_t>>, uint64_t, char_t, true>(options.threshold); });
    bench("LZQGram<8, Hash<8_Mi>>", [](){ return LZQGram<qgram::HashProcessor<8_Mi, uint64_t>, uint64_t, char_t, true>(options.threshold); });
    bench("LZQGram<8, Sketch<6_Mi, 512_Ki x 4>>", [](){ return LZQGram<qgram::SketchProcessor<6_Mi, 512_Ki, 4, uint64_t>, uint64_t, char_t, true>(options.threshold); });
    bench("LZQGram<16, Noop>", [](){ return LZQGram<qgram::NoopProcessor, uint128_t, char_t, true>(options.threshold); });
    bench("LZQGram<16, Trie<UL>>", [](){ return LZQGram<qgram::TrieProcessor<TrieList<char_t>>, uint128_t, char_t, true>(options.threshold); });
    bench("LZQGram<16, Trie<HT>>", [](){ return LZQGram<qgram::TrieProcessor<TrieHash<char_t>>, uint128_t, char_t, true>(options.threshold); });
    bench("LZQGram<16, Hash<6_Mi>>", [](){ return LZQGram<qgram::HashProcessor<6_Mi, uint128_t>, uint128_t, char_t, true>(options.threshold); });
    bench("LZQGram<16, Sketch<1_Mi, 512_Ki x 4>>", [](){ return LZQGram<qgram::SketchProcessor<1_Mi, 512_Ki, 4, uint128_t>, uint128_t, char_t, true>(options.threshold); });
}
