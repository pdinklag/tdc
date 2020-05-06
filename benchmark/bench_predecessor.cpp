#include <iostream>
#include <vector>

#include <tdc/random/permutation.hpp>
#include <tdc/random/vector.hpp>
#include <tdc/stat/phase.hpp>

#include <tdc/pred/binary_search.hpp>
#include <tdc/pred/binary_search_hybrid.hpp>
#include <tdc/pred/index.hpp>
#include <tdc/pred/octrie.hpp>
#include <tdc/pred/octrie_top.hpp>

#include <tlx/cmdline_parser.hpp>

using namespace tdc;

struct {
    size_t num = 1'000'000ULL;
    std::vector<uint64_t> data;

    size_t universe = 0;
    
    size_t num_queries = 10'000'000ULL;
    std::vector<uint64_t> queries;

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
    using pred_t = decltype(constructor(options.data));
    pred_t pred;

    stat::Phase::wrap("construct", [&](){
        pred = constructor(options.data);
    });
    stat::Phase::wrap("predecessor_rnd", [&pred](stat::Phase& phase){
        uint64_t chk = 0;
        for(size_t j = 0; j < options.num_queries; j++) {
            const uint64_t x = options.queries[j];
            auto r = pred.predecessor(options.data.data(), options.num, x);
            chk += r.pos;
        }
        
        auto guard = phase.suppress();
        phase.log("chk", chk);
    });

    if(options.check) {
        size_t num_errors = 0;
        for(size_t j = 0; j < options.num_queries; j++) {
            const uint64_t x = options.queries[j];
            auto r = pred.predecessor(options.data.data(), options.num, x);
            
            assert(r.exists);
            
            // make sure that
            // - x is greater than or equal to the found item
            // - the next item is greater than x
            if(x >= options.data[r.pos] && (r.pos == options.num-1 || options.data[r.pos + 1] > x)) {
                // OK
            } else {
                // nah, count an error
                ++num_errors;
            }
        }
        result.log("errors", num_errors);
    }
}

template<typename C>
void bench(const std::string& name, C constructor) {
    auto result = benchmark_phase("");
 
    bench(constructor, result);
    
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

    // generate numbers
    {
        auto perm = random::Permutation(options.universe, options.seed);
        options.data = perm.vector(options.num);
        std::sort(options.data.begin(), options.data.end());
    }

    // generate query keys, ensuring that there is always a real predecessor (e.g., min <= key < max)
    options.queries = random::vector_range<uint64_t>(options.num_queries, options.data[0], options.data[options.num - 1] - 1, options.seed);
    
    // benchmark
    bench("BinarySearch", [](const std::vector<uint64_t>& data){ return pred::BinarySearch(data.data(), data.size()); });
    bench("BinarySearchHybrid", [](const std::vector<uint64_t>& data){ return pred::BinarySearchHybrid(data.data(), data.size()); });
    bench("Octrie", [](const std::vector<uint64_t>& data){ return pred::Octrie(data.data(), data.size()); });
    bench("OctrieTop(2)", [](const std::vector<uint64_t>& data){ return pred::OctrieTop(data.data(), data.size(), 2); });
    bench("OctrieTop(3)", [](const std::vector<uint64_t>& data){ return pred::OctrieTop(data.data(), data.size(), 3); });
    bench("OctrieTop(4)", [](const std::vector<uint64_t>& data){ return pred::OctrieTop(data.data(), data.size(), 4); });
    bench("Index(4)", [](const std::vector<uint64_t>& data){ return pred::Index(data.data(), data.size(), 4); });
    bench("Index(5)", [](const std::vector<uint64_t>& data){ return pred::Index(data.data(), data.size(), 5); });
    bench("Index(6)", [](const std::vector<uint64_t>& data){ return pred::Index(data.data(), data.size(), 6); });
    bench("Index(7)", [](const std::vector<uint64_t>& data){ return pred::Index(data.data(), data.size(), 7); });
    bench("Index(8)", [](const std::vector<uint64_t>& data){ return pred::Index(data.data(), data.size(), 8); });
    bench("Index(9)", [](const std::vector<uint64_t>& data){ return pred::Index(data.data(), data.size(), 9); });
    return 0;
}
