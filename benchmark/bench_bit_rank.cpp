#include <iostream>
#include <memory>
#include <vector>
#include <utility>

#include <tdc/random/vector.hpp>
#include <tdc/stat/phase.hpp>
#include <tdc/vec/bit_vector.hpp>
#include <tdc/vec/bit_rank.hpp>

#include <tlx/cmdline_parser.hpp>

using namespace tdc;

struct {
    size_t num = 1'000'000ULL;
    std::shared_ptr<vec::BitVector> bits;
    
    size_t num_queries = 10'000'000ULL;
    std::vector<size_t> queries;

    uint64_t seed = random::DEFAULT_SEED;
    
    bool check = false;
    std::vector<size_t> naive;
} options;

stat::Phase benchmark_phase(std::string&& title) {
    stat::Phase phase(std::move(title));
    phase.log("num", options.num);
    phase.log("queries", options.num_queries);
    phase.log("seed", options.seed);
    return phase;
}

template<typename C>
void bench(C constructor, stat::Phase& result) {
    using rank_t = decltype(constructor(options.bits));
    rank_t rank;
    
    stat::Phase::wrap("construct", [&](){
        rank = constructor(options.bits);
    });
    stat::Phase::wrap("rank_rnd", [&](stat::Phase& phase){
        uint64_t chk = 0;
        for(size_t j = 0; j < options.num_queries; j++) {
            chk += rank(options.queries[j]);
        }
        
        auto guard = phase.suppress();
        phase.log("chk", chk);
    });
    
    if(options.check) {
        size_t num_errors = 0;
        for(size_t j = 0; j < options.num_queries; j++) {
            const size_t i = options.queries[j];
            const size_t ds  = rank(i);
            const size_t ref = options.naive[i];
            if(ds != ref) {
                ++num_errors;
            }
        }
        result.log("errors", num_errors);
    }
}

template<uint64_t supblock_width>
void bench_tdc() {
    auto result = benchmark_phase("result");
 
    bench([](std::shared_ptr<const vec::BitVector> bv){ return vec::BitRank<supblock_width>(bv); }, result);
    
    result.suppress([&](){
        std::cout << "RESULT algo=tdc<" << supblock_width << "> " << result.to_keyval() << " " << result.subphases_keyval() << " " << result.subphases_keyval("chk") << std::endl;
    });
}

int main(int argc, char** argv) {
    tlx::CmdlineParser cp;
    cp.add_bytes('n', "num", options.num, "The size of the bit vetor (default: 1M).");
    cp.add_bytes('q', "queries", options.num_queries, "The size of the bit vetor (default: 10M).");
    cp.add_bytes('s', "seed", options.seed, "The random seed.");
    cp.add_flag("check", options.check, "Check results for correctness.");
    if(!cp.process(argc, argv)) {
        return -1;
    }

    // generate bits
    {
        auto bits = random::vector<bool>(options.num, 1, options.seed);
        options.bits = std::make_shared<vec::BitVector>(bits);
    }

    // generate queries
    options.queries = random::vector<size_t>(options.num_queries, options.num - 1, options.seed);

    // prepare naive check structure
    if(options.check) {
        options.naive = std::vector<size_t>(options.num);
        
        size_t rank = 0;
        for(size_t i = 0; i < options.num; i++) {
            rank += (*options.bits)[i];
            options.naive[i] = rank;
        }
    }
    
    // benchmark
    bench_tdc<8>();
    bench_tdc<9>();
    bench_tdc<10>();
    bench_tdc<11>();
    bench_tdc<12>();
    bench_tdc<13>();
    bench_tdc<14>();
    bench_tdc<15>();
    bench_tdc<16>();
    return 0;
}
