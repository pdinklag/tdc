#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

#include <tdc/comp/lz78/binary_trie.hpp>
#include <tdc/comp/lz78/lz78.hpp>
#include <tdc/comp/lz78/stats.hpp>

#include <tdc/io/null_ostream.hpp>
#include <tdc/stat/phase.hpp>
#include <tdc/util/literals.hpp>

#include <tlx/cmdline_parser.hpp>

using namespace tdc::comp::lz78;

using char_t = unsigned char;

struct {
    std::string filename;
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
        phase.log("trie_size", stats.trie_size);
        phase.log("num_refs", stats.trie_size);
        phase.log("num_literals", 0); // for comparison to LZ77
        // std::cout << std::endl;
        std::cout << "RESULT algo=" << name << " group=" << group << " input=" << options.filename << " " << phase.to_keyval() << std::endl;
    }
}

int main(int argc, char** argv) {
    tlx::CmdlineParser cp;
    cp.add_param_string("file", options.filename, "The input file.");
    cp.add_string('a', "group", options.ds, "The algorithm group to benchmark.");
    if(!cp.process(argc, argv)) {
        return -1;
    }
    
    bench("base", "LZ78(FIFO)", [](){ return LZ78<BinaryTrie<char>, true>(); });
    bench("base", "LZ78(MTF)", [](){ return LZ78<BinaryTrie<char, true>, true>(); });
}
