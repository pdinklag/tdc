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

int main(int argc, char** argv) {
    tlx::CmdlineParser cp;
    cp.add_bytes('n', "num", options.num, "The size of the bit vetor (default: 1M).");
    cp.add_bytes('q', "queries", options.num_queries, "The size of the bit vetor (default: 10M).");
    cp.add_bytes('s', "seed", options.seed, "The random seed.");
    if(!cp.process(argc, argv)) {
        return -1;
    }

    // generate bits
	std::shared_ptr<vec::BitVector> bv;
	{
		auto bits = random::vector<bool>(options.num, 1, options.seed);
		bv = std::make_shared<vec::BitVector>(bits);
	}

    // generate queries
    options.queries = random::vector<size_t>(options.num_queries, options.num - 1, options.seed);

    // build rank data structure
	vec::BitRank rank(bv);
	
	// benchmark
    {
		vec::BitRank rank;
        auto result = benchmark_phase("rank");
     
		stat::Phase::wrap("construct", [&](){
			rank = vec::BitRank(bv);
		});
		stat::Phase::wrap("rank_rnd", [&](stat::Phase& phase){
			uint64_t chk = 0;
			for(size_t j = 0; j < options.num_queries; j++) {
				chk += rank(options.queries[j]);
			}
			
			auto guard = phase.suppress();
			phase.log("chk", chk);
		});
		
        result.suppress([&](){
            std::cout << "RESULT algo=std " << result.to_keyval() << " " << result.subphases_keyval() << " " << result.subphases_keyval("chk") << std::endl;
        });
    }

    
    return 0;
}
