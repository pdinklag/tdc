#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

#include <tdc/comp/lz77/noop.hpp>
#include <tdc/comp/lz77/lz77_sa.hpp>
#include <tdc/comp/lz77/lzqgram.hpp>
#include <tdc/comp/lz77/lzqgram_btree.hpp>
#include <tdc/comp/lz77/lzqgram_hash.hpp>
#include <tdc/comp/lz77/lzqgram_sketch.hpp>
#include <tdc/comp/lz77/lzqgram_sketch2.hpp>
#include <tdc/comp/lz77/lzqgram_sketch3.hpp>
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
        phase.log("input_size", stats.input_size);
        phase.log("num_literals", stats.num_literals);
        phase.log("num_refs", stats.num_refs);
        phase.log("num_collisions", stats.num_collisions);
        phase.log("num_extensions", stats.num_extensions);
        phase.log("extension_sum", stats.extension_sum);
        phase.log("num_swaps", stats.num_swaps);
        phase.log("trie_size", stats.trie_size);
        // std::cout << std::endl;
        std::cout << "RESULT algo=" << name << " group=" << group << " input=" << options.filename << " threshold=" << options.threshold << " " << phase.to_keyval() << std::endl;
    }
}

int main(int argc, char** argv) {
    tlx::CmdlineParser cp;
    cp.add_param_string("file", options.filename, "The input file.");
    cp.add_size_t('t', "threshold", options.threshold, "Minimum reference length.");
    cp.add_string('a', "group", options.ds, "The algorithm group to benchmark.");
    cp.add_size_t('q', "min-qgram", options.q, "The minimum q-gram length.");
    if(!cp.process(argc, argv)) {
        return -1;
    }
    
    bench("base", "Noop", [](){ return Noop<true>(); });
    bench("base", "SA", [](){ return LZ77SA<true>(options.threshold); });
    
    if(options.q == 0 || options.q == 8) {
        bench("btree", "BTree(64),q=8", [](){ return LZQGram<qgram::BTreeProcessor<64, uint64_t>, uint64_t, char_t, std::endian::little, true>(options.threshold); });
        bench("hash", "Hash(40_Mi),q=8", [](){ return LZQGram<qgram::HashProcessor<40_Mi, uint64_t>, uint64_t, char_t, std::endian::little, true>(options.threshold); });
        bench("sketch", "Sketch(f=32Mi,w=2Mi,d=4),q=8", [](){ return LZQGram<qgram::SketchProcessor<32_Mi, 2_Mi, 4, uint64_t>, uint64_t, char_t, std::endian::little, true>(options.threshold); });
        bench("sketch", "Sketch2(f=32Mi,w=2Mi,d=4),q=8", [](){ return LZQGram<qgram::SketchProcessor2<32_Mi, 2_Mi, 4, uint64_t>, uint64_t, char_t, std::endian::little, true>(options.threshold); });
        bench("sketch", "Sketch3(f=32Mi,w=2Mi,d=4),q=8", [](){ return LZQGram<qgram::SketchProcessor3<32_Mi, 2_Mi, 4, uint64_t>, uint64_t, char_t, std::endian::little, true>(options.threshold); });
        bench("trie", "Trie(MTF),q=8", [](){ return LZQGram<qgram::TrieProcessor<char_t, true>, uint64_t, char_t, std::endian::little, true>(options.threshold); });
    }
    
    if(options.q == 0 || options.q == 16) {
        bench("btree", "BTree(64),q=16", [](){ return LZQGram<qgram::BTreeProcessor<64, uint128_t>, uint128_t, char_t, std::endian::little, true>(options.threshold); });
        bench("hash", "Hash(28_Mi),q=16", [](){ return LZQGram<qgram::HashProcessor<24_Mi, uint128_t>, uint128_t, char_t, std::endian::little, true>(options.threshold); });
        bench("sketch", "Sketch(f=4Mi,w=512Ki,d=4),q=16", [](){ return LZQGram<qgram::SketchProcessor<4_Mi, 512_Ki, 4, uint128_t>, uint128_t, char_t, std::endian::little, true>(options.threshold); });
        bench("sketch", "Sketch2(f=4Mi,w=512Ki,d=4),q=16", [](){ return LZQGram<qgram::SketchProcessor2<4_Mi, 512_Ki, 4, uint128_t>, uint128_t, char_t, std::endian::little, true>(options.threshold); });
        bench("sketch", "Sketch3(f=4Mi,w=512Ki,d=4),q=16", [](){ return LZQGram<qgram::SketchProcessor3<4_Mi, 512_Ki, 4, uint128_t>, uint128_t, char_t, std::endian::little, true>(options.threshold); });
        bench("trie", "Trie(MTF),q=16", [](){ return LZQGram<qgram::TrieProcessor<char_t, true>, uint128_t, char_t, std::endian::little, true>(options.threshold); });
    }
}
