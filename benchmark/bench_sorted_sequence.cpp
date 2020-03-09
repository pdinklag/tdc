#include <algorithm>
#include <iostream>
#include <vector>

#include <tdc/random/permutation.hpp>
#include <tdc/random/vector.hpp>
#include <tdc/stat/phase.hpp>
#include <tdc/vec/sorted_sequence.hpp>

#include <tlx/cmdline_parser.hpp>

using namespace tdc;

struct {
    size_t num = 1'000'000ULL;
    std::vector<uint64_t> data;

    size_t universe = 0;
    
    size_t num_queries = 10'000'000ULL;
    std::vector<size_t> queries;

    uint64_t seed = random::DEFAULT_SEED;

    bool check = false;
} options;

stat::Phase benchmark_phase(std::string&& title) {
    stat::Phase phase(std::move(title));
    phase.log("num", options.num);
    phase.log("universe", options.universe);
    phase.log("queries", options.num_queries);
    phase.log("seed", options.seed);
    return phase;
}

template<typename C>
void bench(C constructor, stat::Phase& result) {
    using seq_t = decltype(constructor(options.data));
    seq_t seq;

    stat::Phase::wrap("construct", [&](){
        seq = constructor(options.data);
    });
    stat::Phase::wrap("get_rnd", [&seq](stat::Phase& phase){
        uint64_t chk = 0;
        for(size_t j = 0; j < options.num_queries; j++) {
            const size_t i = options.queries[j];
            chk += seq[i];
        }
        
        auto guard = phase.suppress();
        phase.log("chk", chk);
    });

    if(options.check) {
        size_t num_errors = 0;
        for(size_t j = 0; j < options.num_queries; j++) {
            const size_t i = options.queries[j];
            const size_t ds  = seq[i];
            const size_t ref = options.data[i];
            if(ds != ref) {
                ++num_errors;
            }
        }
        result.log("errors", num_errors);
    }
}

int main(int argc, char** argv) {
    tlx::CmdlineParser cp;
    cp.add_bytes('n', "num", options.num, "The length of the sequence (default: 1M).");
    cp.add_bytes('u', "universe", options.universe, "The size of the universe to draw from (default: 10 * n)");
    cp.add_bytes('q', "queries", options.num_queries, "The size of the bit vetor (default: 10M).");
    cp.add_bytes('s', "seed", options.seed, "The random seed.");
    cp.add_flag("check", options.check, "Check results for correctness.");
    if(!cp.process(argc, argv)) {
        return -1;
    }

    if(!options.universe) {
        options.universe = 10 * options.num;
    }

    // generate numbers
    {
        auto perm = random::Permutation(options.universe, options.seed);
        options.data = perm.vector(options.num);
        std::sort(options.data.begin(), options.data.end());
    }

    // generate queries
    options.queries = random::vector<size_t>(options.num_queries, options.num - 1, options.seed);
    
    // benchmark
    {
        auto result = benchmark_phase("std");
     
        bench([](const std::vector<uint64_t>& data){ return data; }, result);
        
        result.suppress([&](){
            std::cout << "RESULT algo=std<uint64_t> " << result.to_keyval() << " " << result.subphases_keyval() << " " << result.subphases_keyval("chk") << std::endl;
        });
    }
    {
        auto result = benchmark_phase("tdc");
     
        bench([](const std::vector<uint64_t>& data){ return vec::SortedSequence(data.data(), data.size()); }, result);
        
        result.suppress([&](){
            std::cout << "RESULT algo=SortedSequence " << result.to_keyval() << " " << result.subphases_keyval() << " " << result.subphases_keyval("chk") << std::endl;
        });
    }
    
    return 0;
}
