#include <iostream>
#include <set>
#include <vector>

#include <tdc/random/permutation.hpp>
#include <tdc/random/vector.hpp>
#include <tdc/stat/phase.hpp>

#include <tdc/pred/dynamic/dynamic_octrie.hpp>

#include <tlx/cmdline_parser.hpp>

using namespace tdc;

struct {
    size_t num = 1'000'000ULL;
    size_t universe = 0;
    size_t num_queries = 10'000'000ULL;
    uint64_t seed = random::DEFAULT_SEED;
} options;

stat::Phase benchmark_phase(std::string&& title) {
    stat::Phase phase(std::move(title));
    phase.log("num", options.num);
    phase.log("universe", options.universe);
    phase.log("queries", options.num_queries);
    phase.log("seed", options.seed);
    return phase;
}

int main(int argc, char** argv) {
    tlx::CmdlineParser cp;
    cp.add_bytes('n', "num", options.num, "The length of the sequence (default: 1M).");
    cp.add_bytes('u', "universe", options.universe, "The size of the universe to draw from (default: 10 * n)");
    cp.add_bytes('q', "queries", options.num_queries, "The number to draw from the universe (default: 10M).");
    cp.add_bytes('s', "seed", options.seed, "The random seed.");
    if(!cp.process(argc, argv)) {
        return -1;
    }

    if(!options.universe) {
        options.universe = 10 * options.num;
    }
    
    // generate permutation
    auto perm = random::Permutation(options.universe, options.seed);
    uint64_t min = UINT64_MAX;
    uint64_t max = 0;
    for(size_t i = 0; i < options.num; i++) {
        const uint64_t x = perm(i);
        min = std::min(min, x);
        max = std::min(max, x);
    }
    
    auto qperm = random::Permutation(max - min, options.seed ^ 0x1234ABCD);

    // DynamicOctrie
    {
        auto result = benchmark_phase("");
     
        pred::dynamic::DynamicOctrie trie;
        
        // insert
        {
            stat::Phase::wrap("insert", [&](){
                for(size_t i = 0; i < options.num; i++) {
                    trie.insert(perm(i));
                }
            });
        }
        
        // predecessor
        {
            uint64_t chk = 0;
            stat::Phase::wrap("predecessor_rnd", [&](stat::Phase& phase){
                for(size_t i = 0; i < options.num_queries; i++) {
                    const uint64_t x = min + perm(i);
                    auto r = trie.predecessor(x);
                    chk += r.pos;
                }
                
                auto guard = phase.suppress();
                phase.log("chk", chk);
            });
            // TODO: check?
        }
        
        result.suppress([&](){
            std::cout << "RESULT algo=DynamicOctrie " << result.to_keyval() << " " << result.subphases_keyval() << " " << result.subphases_keyval("chk") << std::endl;
        });
    }
    
    // std::set
    {
        auto result = benchmark_phase("");
     
        std::set<uint64_t> set;
        
        // insert
        {
            stat::Phase::wrap("insert", [&](){
                for(size_t i = 0; i < options.num; i++) {
                    set.insert(perm(i));
                }
            });
        }
        
        // predecessor
        {
            uint64_t chk = 0;
            stat::Phase::wrap("predecessor_rnd", [&](stat::Phase& phase){
                for(size_t i = 0; i < options.num_queries; i++) {
                    const uint64_t x = min + perm(i);
                    
                    auto it = set.upper_bound(x);
                    --it;
                    chk += *it;
                }
                
                auto guard = phase.suppress();
                phase.log("chk", chk);
            });
        }
        
        result.suppress([&](){
            std::cout << "RESULT algo=std::set " << result.to_keyval() << " " << result.subphases_keyval() << " " << result.subphases_keyval("chk") << std::endl;
        });
    }
    
    return 0;
}
    