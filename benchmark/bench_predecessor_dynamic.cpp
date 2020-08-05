#include <iostream>
#include <set>
#include <vector>

#include <tdc/math/ilog2.hpp>
#include <tdc/random/permutation.hpp>
#include <tdc/random/vector.hpp>
#include <tdc/stat/phase.hpp>

#include <tdc/pred/binary_search.hpp>
#include <tdc/pred/dynamic/dynamic_index.hpp>

#include <tdc/pred/dynamic/dynamic_octrie.hpp>
#include <tdc/pred/dynamic/dynamic_rankselect.hpp>

#include <tlx/cmdline_parser.hpp>

#if defined(LEDA_FOUND) && defined(STREE_FOUND)
    #define BENCH_STREE
    #include <veb/STree_orig.h>
#endif

using namespace tdc;

constexpr uint64_t OP_INSERT = 0;
constexpr uint64_t OP_QUERY  = 1;
constexpr uint64_t OP_DELETE = 2;

struct {
    size_t num = 1'000'000ULL;
    size_t universe = 0;
    size_t num_queries = 1'000'000ULL;
    uint64_t seed = random::DEFAULT_SEED;
    
    std::string ds; // if non-empty, only benchmarks the selected data structure
    
    bool do_bench(const std::string& name) {
        return ds.length() == 0 || name == ds;
    }

    random::Permutation perm_values;  // value permutation
    random::Permutation perm_queries; // query permutation
    
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

/// \brief Performs a benchmark.
/// \param name        the algorithm name
/// \param ctor_func   constructor function, must support signature T(const uint64_t) and return an empty data structure,
///                    or a data structure containing the first element if it cannot be empty
/// \param size_func   size function, must support signature size_t(const T& ds)
/// \param insert_func insertion function, must support signature <any>(T& ds, const uint64_t x)
/// \param pred_func   predecessor function, must support signature pred::Result(const T& ds, const uint64_t x)
/// \param remove_func key removal function, must support signature <any>(T& ds, const uint64_t x)
template<typename ctor_func_t, typename size_func_t, typename insert_func_t, typename pred_func_t, typename remove_func_t>
void bench(
    const std::string& name,
    ctor_func_t ctor_func,
    size_func_t size_func,
    insert_func_t insert_func,
    pred_func_t pred_func,
    remove_func_t remove_func
) {
        
    if(!options.do_bench(name)) return;

    // measure
    auto result = benchmark_phase("");
    {
        // construct data structure so it contains only zero
        auto ds = ctor_func(0);
        if(size_func(ds) == 0) insert_func(ds, 0);
        
        assert(size_func(ds) == 1);

        // insert
        {
            stat::Phase::MemoryInfo mem;
            {
                stat::Phase insert("insert");
                for(size_t i = 0; i < options.num; i++) {
                    insert_func(ds, options.perm_values(i) + 1);  // add 1 because zero is already in
                }
                mem = insert.memory_info();
            }
            result.log("memData", mem.current - mem.offset);
        }
        
        // make sure all have been inserted
        assert(size_func(ds) == options.num+1);
        
        // predecessor queries
        {
            uint64_t chk_q = 0;
            {
                stat::Phase phase("predecessor_rnd");
                for(size_t i = 0; i < options.num_queries; i++) {
                    chk_q += pred_func(ds, options.perm_queries(i)).pos;
                }
            }
            result.log("chk_q", chk_q);
        }

        // check
        if(options.check) {
            size_t num_errors = 0;
            for(size_t j = 0; j < options.num_queries; j++) {
                const uint64_t x = options.perm_queries(j);
                auto r = pred_func(ds, x);
                assert(r.exists);
                
                // make sure that the result equals that of a simple binary search on the input
                auto correct_result = options.data_pred.predecessor(options.data.data(), options.num, x);
                assert(correct_result.exists);
                if(r.pos == options.data[correct_result.pos]) {
                    // OK
                } else {
                    // nah, count an error
                    //std::cout << std::hex << "index: " << x << "  correct: " << options.data[correct_result.pos] << "  wrong: " << r.pos << std::endl;
                    ++num_errors;
                }
            }
            result.log("errors", num_errors);
        }
        
        // delete
        {
            stat::Phase del("delete");
            for(size_t i = 0; i < options.num; i++) {
                remove_func(ds, options.perm_values(i) + 1); // add 1 to keep zero in there
            }
        }

        // make sure size is back to normal (only zero is contained)
        assert(size_func(ds) == 1);
        
        // memory of empty data structure
        {
            auto mem = result.memory_info();
            result.log("memEmpty", mem.current - mem.offset);
        }
    }
    
    std::cout << "RESULT algo=" << name << " " << result.to_keyval() << " " << result.subphases_keyval() << std::endl;
}

int main(int argc, char** argv) {
    tlx::CmdlineParser cp;
    cp.add_bytes('n', "num", options.num, "The length of the sequence (default: 1M).");
    cp.add_bytes('u', "universe", options.universe, "The base-2 logarithm of the universe to draw from (default: 2x num)");
    cp.add_bytes('q', "queries", options.num_queries, "The number to draw from the universe (default: 1M).");
    cp.add_bytes('s', "seed", options.seed, "The random seed.");
    cp.add_string("ds", options.ds, "The data structure to benchmark. If omitted, all data structures are benchmarked.");
    cp.add_flag("check", options.check, "Check results for correctness.");
    
    if(!cp.process(argc, argv)) {
        return -1;
    }

    if(!options.universe) {
        options.universe = 2 * (options.num);
    } else {
        options.universe = (options.universe == 64) ? UINT64_MAX : (1ULL << options.universe)-1;
        if(options.universe < options.num + 1) {
            std::cerr << "universe not large enough" << std::endl;
            return -1;
        }
    }

    // generate permutation
    // we subtract 1 from the universe because we add it back for the insertions
    options.perm_values = random::Permutation(options.universe - 1, options.seed);

    if(options.check) {
        // insert keys
        options.data.reserve(options.num);
        options.data.push_back(0);
        for(size_t i = 0; i < options.num; i++) {
            options.data.push_back(options.perm_values(i) + 1); // add one because zero is already in
        }

        // prepare verification
        std::sort(options.data.begin(), options.data.end());
        options.data_pred = pred::BinarySearch(options.data.data(), options.num);
    }
    
    options.perm_queries = random::Permutation(options.universe, options.seed ^ 0x1234ABCD);

    bench("fusion_btree",
        [](const uint64_t){ return pred::dynamic::DynamicOctrie(); },
        [](const auto& ds){ return ds.size(); },
        [](auto& ds, const uint64_t x){ ds.insert(x); },
        [](const auto& ds, const uint64_t x){ return ds.predecessor(x); },
        [](auto& ds, const uint64_t x){ ds.remove(x); }
    );
    
    bench("index_hybrid",
        [](const uint64_t){ return pred::dynamic::DynIndex<tdc::pred::dynamic::bucket_hybrid, 16>(); },
        [](const auto& ds){ return ds.size(); },
        [](auto& ds, const uint64_t x){ ds.insert(x); },
        [](const auto& ds, const uint64_t x){ return ds.predecessor(x); },
        [](auto& ds, const uint64_t x){ ds.remove(x); }
    );
    /*   
    bench("index_bv",
        [](const uint64_t){ return pred::dynamic::DynIndex<tdc::pred::dynamic::bucket_bv, 16>(); },
        [](const auto& ds){ return ds.size(); },
        [](auto& ds, const uint64_t x){ ds.insert(x); },
        [](const auto& ds, const uint64_t x){ return ds.predecessor(x); },
        [](auto& ds, const uint64_t x){ ds.remove(x); }
    );
        
    bench("index_list",
        [](const uint64_t){ return pred::dynamic::DynIndex<tdc::pred::dynamic::bucket_list, 16>(); },
        [](const auto& ds){ return ds.size(); },
        [](auto& ds, const uint64_t x){ ds.insert(x); },
        [](const auto& ds, const uint64_t x){ return ds.predecessor(x); },
        [](auto& ds, const uint64_t x){ ds.remove(x); }
    );
    */
    
    bench("set",
        [](const uint64_t){ return std::set<uint64_t>(); },
        [](const auto& set){ return set.size(); },
        [](auto& set, const uint64_t x){ set.insert(x); },
        [](const auto& set, const uint64_t x){
            auto it = set.upper_bound(x);
            return pred::Result { it != set.begin(), *(--it) };
        },
        [](auto& set, const uint64_t x){ set.erase(x); }
    );
        
#ifdef PLADS_FOUND
    // TOO SLOW
    /*
    bench("dbv",
        [](const uint64_t){ return pred::dynamic::DynamicRankSelect(); },
        [](const auto& dbv){ return dbv.size(); },
        [](auto& dbv, const uint64_t x){ dbv.insert(x); },
        [](const auto& dbv, const uint64_t x){ return dbv.predecessor(x); },
        [](auto& dbv, const uint64_t x){ dbv.remove(x); }
    );
    */
#endif

#ifdef BENCH_STREE
    if(options.universe <= INT32_MAX) {
        bench("stree",
            [](const uint64_t first){
                // STree cannot be empty
                const size_t k = math::ilog2_ceil(options.universe);
                return STree_orig<>(k, first);
            },
            [](auto& stree){ return stree.getSize(); },
            [](auto& stree, const uint64_t x){ stree.insert(x); },
            [](auto& stree, const uint64_t x){
                // STree seems to look for the largest value STRICTLY LESS THAN the input
                // and crashes if there is no predecessor...
                return pred::Result { true, (size_t)stree.pred(x+1) };
            },
            [](auto& stree, const uint64_t x){ stree.del(x); }
        );
    } else {
        std::cerr << "WARNING: STree only supports 31-bit universes and will therefore not be benchmarked" << std::endl;
    }
#endif

    return 0;
}
    
