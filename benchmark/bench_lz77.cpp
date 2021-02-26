#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

#include <tdc/comp/lz77/lzsketch.hpp>

#include <tdc/uint/uint128.hpp>
#include <tdc/io/null_ostream.hpp>
#include <tdc/stat/phase.hpp>

#include <tlx/cmdline_parser.hpp>

using namespace tdc::comp::lz77;

struct {
    std::string filename;
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
            c.compress(input, devnull);
            stats = c.stats();
        }
        
        auto guard = phase.suppress();
        phase.log("input_size", stats.input_size);
        phase.log("num_literals", stats.num_literals);
        phase.log("num_refs", stats.num_refs);
        phase.log("filter_size", stats.debug);
        std::cout << "RESULT algo=" << name << " " << phase.to_keyval() << std::endl;
    }
}

int main(int argc, char** argv) {
    tlx::CmdlineParser cp;
    cp.add_param_string("file", options.filename, "The input file.");
    if(!cp.process(argc, argv)) {
        return -1;
    }
    
    bench("LZSketch<32>(2)", [](){ return LZSketch<unsigned char, uint32_t, true>(2); });
    bench("LZSketch<32>(3)", [](){ return LZSketch<unsigned char, uint32_t, true>(3); });
    bench("LZSketch<32>(4)", [](){ return LZSketch<unsigned char, uint32_t, true>(4); });
    bench("LZSketch<64>(2)", [](){ return LZSketch<unsigned char, uint64_t, true>(2); });
    bench("LZSketch<64>(3)", [](){ return LZSketch<unsigned char, uint64_t, true>(3); });
    bench("LZSketch<64>(4)", [](){ return LZSketch<unsigned char, uint64_t, true>(4); });
    bench("LZSketch<64>(5)", [](){ return LZSketch<unsigned char, uint64_t, true>(5); });
    bench("LZSketch<64>(6)", [](){ return LZSketch<unsigned char, uint64_t, true>(6); });
    bench("LZSketch<64>(7)", [](){ return LZSketch<unsigned char, uint64_t, true>(6); });
    bench("LZSketch<64>(8)", [](){ return LZSketch<unsigned char, uint64_t, true>(8); });
}
