#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

#include <tdc/comp/lz77/lz77_sa.hpp>
#include <tdc/comp/lz77/lzqgram_hash.hpp>
#include <tdc/comp/lz77/lzqgram_table.hpp>
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
        phase.log("debug", stats.debug);
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
    
    bench("LZ77SA()", [](){ return LZ77SA<true>(options.threshold); });
    
    bench("LZQGramTable(8, 256_Ki x 1)", [](){ return LZQGramTable<char_t, uint64_t, true>(256_Ki, 1, options.threshold); });
    bench("LZQGramTable(8, 256_Ki x 2)", [](){ return LZQGramTable<char_t, uint64_t, true>(256_Ki, 2, options.threshold); });
    bench("LZQGramTable(8, 256_Ki x 4)", [](){ return LZQGramTable<char_t, uint64_t, true>(256_Ki, 4, options.threshold); });
    bench("LZQGramTable(8, 256_Ki x 8)", [](){ return LZQGramTable<char_t, uint64_t, true>(256_Ki, 8, options.threshold); });
    bench("LZQGramTable(8, 256_Ki x 16)", [](){ return LZQGramTable<char_t, uint64_t, true>(256_Ki, 16, options.threshold); });
    bench("LZQGramTable(8, 256_Ki x 32)", [](){ return LZQGramTable<char_t, uint64_t, true>(256_Ki, 32, options.threshold); });
}
