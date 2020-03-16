#include <filesystem>
#include <iostream>
#include <sstream>

#include <tdc/code/binary_coder.hpp>
#include <tdc/io/bit_ostream.hpp>
#include <tdc/io/bit_istream.hpp>
#include <tdc/io/load_file.hpp>
#include <tdc/math/idiv.hpp>
#include <tdc/stat/phase.hpp>

#include <tdc/code/binary_coder.hpp>
#include <tdc/code/delta0_coder.hpp>
#include <tdc/code/rice_coder.hpp>

#include <tlx/cmdline_parser.hpp>

using namespace tdc;

using char_type = uint8_t;

struct {
    std::string input_file;
    
    std::vector<char_type> input;
} options;

stat::Phase benchmark_phase(std::string&& title) {
    stat::Phase phase(std::move(title));
    phase.log("input", options.input_file);
    phase.log("input_size", options.input.size());
    return phase;
}

template<typename C>
void bench(C constructor, stat::Phase& result) {
    std::string enc;
    {
        auto coder = constructor();

        std::ostringstream oss;
        io::BitOStream out(oss);
        
        stat::Phase::wrap("init", [&](){
            coder.encode_init(out, options.input.data(), options.input.size());
        });
        
        stat::Phase::wrap("encode", [&](){
            for(size_t i = 0; i < options.input.size(); i++) {
                coder.encode(out, options.input[i]);
            }
        });

        result.log("bits_written", out.bits_written());
        result.log("output_size", math::idiv_ceil(out.bits_written(), CHAR_BIT));
        result.log("ratio", (double)out.bits_written() / (double)(options.input.size() * CHAR_BIT * sizeof(char_type)));
    }    
}

int main(int argc, char** argv) {
    tlx::CmdlineParser cp;
    cp.add_param_string("file", options.input_file, "The input filename.");
    if(!cp.process(argc, argv)) {
        return -1;
    }

    // load input
    options.input = io::load_file_as_vector<char_type>(options.input_file);

    // benchmark
    {
        auto result = benchmark_phase("BinaryCoder");
     
        bench([](){ return code::BinaryCoder(CHAR_BIT * sizeof(char_type)); }, result);
        
        result.suppress([&](){
            std::cout << "RESULT algo=BinaryCoder " << result.to_keyval() << " " << result.subphases_keyval() << std::endl;
        });
    }

    {
        auto result = benchmark_phase("Delta0Coder");
     
        bench([](){ return code::Delta0Coder(); }, result);
        
        result.suppress([&](){
            std::cout << "RESULT algo=Delta0Coder " << result.to_keyval() << " " << result.subphases_keyval() << std::endl;
        });
    }

    for(size_t e = 1; e <= 8; e++) {
        auto result = benchmark_phase("RiceCoder");
     
        bench([e](){ return code::RiceCoder(e); }, result);
        
        result.suppress([&](){
            std::cout << "RESULT algo=RiceCoder(" << e << ") " << result.to_keyval() << " " << result.subphases_keyval() << std::endl;
        });
    }
}
