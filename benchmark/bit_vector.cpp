#include <iostream>
#include <vector>

#include <tdc/random/vector.hpp>
#include <tdc/stat/phase.hpp>
#include <tdc/vec/bit_vector.hpp>

#include <tlx/cmdline_parser.hpp>

using namespace tdc;

struct {
    size_t num = 1'000'000ULL;
    std::vector<bool> data;
    
    size_t num_queries = 10'000'000ULL;
    std::vector<size_t> queries;

    uint64_t seed = random::DEFAULT_SEED;
} options;

stat::Phase benchmark_phase(std::string&& title) {
    stat::Phase phase(std::move(title));
    phase.log("num", options.num);
    phase.log("queries", options.num_queries);
    phase.log("seed", options.seed);
    return phase;
}

template<typename bit_vector_t>
void bench() {
    bit_vector_t bv(options.num);
    
    stat::Phase::wrap("set_seq", [&bv](){
        for(size_t i = 0; i < options.num; i++) {
            bv[i] = options.data[i];
        }
    });
    stat::Phase::wrap("get_seq", [&bv](stat::Phase& phase){
        uint64_t chk = 0;
        for(size_t i = 0; i < options.num; i++) {
            chk += bv[i];
        }
        phase.log("chk", chk);
    });
    stat::Phase::wrap("get_rnd", [&bv](stat::Phase& phase){
        uint64_t chk = 0;
        for(size_t j = 0; j < options.num_queries; j++) {
            const size_t i = options.queries[j];
            chk += bv[i];
        }
        phase.log("chk", chk);
    });
    stat::Phase::wrap("set_rnd", [&bv](){
        for(size_t j = 0; j < options.num_queries; j++) {
            const size_t i = options.queries[j];
            bv[i] = bool((i+j) & 1);
        }
    });
}

int main(int argc, char** argv) {
    tlx::CmdlineParser cp;
    cp.add_bytes('n', "num", options.num, "The size of the bit vetor (default: 1M).");
    cp.add_bytes('q', "queries", options.num_queries, "The size of the bit vetor (default: 10M).");
    cp.add_bytes('s', "seed", options.seed, "The random seed.");
    if(!cp.process(argc, argv)) {
        return -1;
    }

    // generate bits
    options.data = random::vector<bool>(options.num, 1, options.seed);

    // generate queries
    options.queries = random::vector<size_t>(options.num_queries, options.num - 1, options.seed);

    // tdc::vec::BitVector
    {
        auto result = benchmark_phase("tdc");
        bench<vec::BitVector>();
        
        result.suppress([&](){
            std::cout << "RESULT algo=tdc " << result.to_keyval() << " " << result.subphases_keyval() << " " << result.subphases_keyval("chk") << std::endl;
        });
    }
    // std::vector<bool>
    {
        auto result = benchmark_phase("std");
        bench<std::vector<bool>>();
        
        result.suppress([&](){
            std::cout << "RESULT algo=std " << result.to_keyval() << " " << result.subphases_keyval() << " " << result.subphases_keyval("chk") << std::endl;
        });
    }
    
    return 0;
}
