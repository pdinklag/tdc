#include <iostream>
#include <set>
#include <vector>

#include <tdc/random/permutation.hpp>
#include <tdc/random/vector.hpp>
#include <tdc/stat/phase.hpp>

#include <tdc/pred/binary_search.hpp>
#include <tdc/pred/dynamic/dynamic_octrie.hpp>

#include <tlx/cmdline_parser.hpp>

using namespace tdc;

struct {
    size_t num = 1'000'000ULL;
    size_t universe = 0;
    size_t num_queries = 10'000'000ULL;
    uint64_t seed = random::DEFAULT_SEED;
    
    bool check;
    std::vector<uint64_t> data; // only used if check == true
    pred::BinarySearch data_pred;
} options;

stat::Phase benchmark_phase(std::string&& title) {
    stat::Phase phase(std::move(title));
    phase.log("num", options.num);
    phase.log("universe", options.universe);
    phase.log("queries", options.num_queries);
    phase.log("seed", options.seed);
    return phase;
}

// pred_ds_t must have an empty constructor and support insert
template<typename pred_ds_t>
void bench(
    const std::string& name,
    std::function<pred::Result(const pred_ds_t& ds, const uint64_t x)> pred,
    const random::Permutation& perm,
    const random::Permutation& qperm,
    const uint64_t qperm_min) {

    // measure
    auto result = benchmark_phase("");
    {
        // construct empty
        pred_ds_t ds;

        // insert
        {
            stat::Phase::wrap("insert", [&](){
                for(size_t i = 0; i < options.num; i++) {
                    ds.insert(perm(i));
                }
            });
        }
        
        // predecessor queries
        {
            uint64_t chk = 0;
            stat::Phase::wrap("predecessor_rnd", [&](stat::Phase& phase){
                for(size_t i = 0; i < options.num_queries; i++) {
                    const uint64_t x = qperm_min + qperm(i);
                    auto r = pred(ds, x);
                    chk += r.pos;
                }
                
                auto guard = phase.suppress();
                phase.log("chk", chk);
            });
        }
        
        // TODO: check
        if(options.check) {
            result.suppress([&](){
                size_t num_errors = 0;
                for(size_t j = 0; j < options.num_queries; j++) {
                    const uint64_t x = qperm_min + qperm(j);
                    auto r = pred(ds, x);
                    assert(r.exists);
                    
                    // make sure that the result equals that of a simple binary search on the input
                    auto correct_result = options.data_pred.predecessor(options.data.data(), options.num, x);
                    assert(correct_result.exists);
                    if(r.pos == options.data[correct_result.pos]) {
                        // OK
                    } else {
                        // nah, count an error
                        ++num_errors;
                    }
                }
                result.log("errors", num_errors);
            });
        }
    }
    
    result.suppress([&](){
        std::cout << "RESULT algo=" << name << " " << result.to_keyval() << " " << result.subphases_keyval() << " " << result.subphases_keyval("chk") << std::endl;
    });
}

int main(int argc, char** argv) {
    tlx::CmdlineParser cp;
    cp.add_bytes('n', "num", options.num, "The length of the sequence (default: 1M).");
    cp.add_bytes('u', "universe", options.universe, "The size of the universe to draw from (default: 10 * n)");
    cp.add_bytes('q', "queries", options.num_queries, "The number to draw from the universe (default: 10M).");
    cp.add_bytes('s', "seed", options.seed, "The random seed.");
    cp.add_flag("check", options.check, "Check results for correctness.");
    
    if(!cp.process(argc, argv)) {
        return -1;
    }

    if(!options.universe) {
        options.universe = 10 * options.num;
    }
    
    // generate permutation
    auto perm = random::Permutation(options.universe, options.seed);
    uint64_t qmin = UINT64_MAX;
    uint64_t qmax = 0;
    
    if(options.check) {
        // data will contain sorted keys
        options.data.reserve(options.num);
    }
    
    for(size_t i = 0; i < options.num; i++) {
        const uint64_t x = perm(i);
        qmin = std::min(qmin, x);
        qmax = std::max(qmax, x);
        
        if(options.check) {
            options.data.push_back(x);
        }
    }
    
    if(options.check) {
        // prepare verification
        std::sort(options.data.begin(), options.data.end());
        options.data_pred = pred::BinarySearch(options.data.data(), options.num);
    }
    
    auto qperm = random::Permutation(qmax - qmin, options.seed ^ 0x1234ABCD);

    bench<std::set<uint64_t>>("set",
        [](const std::set<uint64_t>& set, const uint64_t x){
            auto it = set.upper_bound(x);
            return pred::Result { it != set.begin(), *(--it) };
        },
        perm, qperm, qmin);
        
    bench<pred::dynamic::DynamicOctrie>("DynamicOctrie",
        [](const pred::dynamic::DynamicOctrie& trie, const uint64_t x){
            return trie.predecessor(x);
        },
        perm, qperm, qmin);

    return 0;
}
    
