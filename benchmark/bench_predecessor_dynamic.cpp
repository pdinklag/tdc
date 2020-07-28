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

struct {
    size_t num = 1'000'000ULL;
    size_t universe = 0;
    size_t num_queries = 10'000'000ULL;
    uint64_t seed = random::DEFAULT_SEED;
    
    std::string ds; // if non-empty, only benchmarks the selected data structure
    
    bool do_bench(const std::string& name) {
        return ds.length() == 0 || name == ds;
    }
    
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
/// \param perm        the permutation for inserted values
/// \param qperm       the permutation for queries
/// \param qperm_min   the minimum value inserted into the data structure, used to assure a predecessor query always succeeds
template<typename ctor_func_t, typename size_func_t, typename insert_func_t, typename pred_func_t, typename remove_func_t>
void bench(
    const std::string& name,
    ctor_func_t ctor_func,
    size_func_t size_func,
    insert_func_t insert_func,
    pred_func_t pred_func,
    remove_func_t remove_func,
    const random::Permutation& perm,
    const random::Permutation& qperm,
    const uint64_t qperm_min,
    const bool non_empty = false) {
        
    if(!options.do_bench(name)) return;

    // measure
    auto result = benchmark_phase("");
    {
        // construct empty
        auto ds = ctor_func(perm(0));
        const size_t initial_size = size_func(ds);

        // insert
        {
            stat::Phase::MemoryInfo mem;
            {
                stat::Phase insert("insert");
                for(size_t i = initial_size; i < options.num; i++) {
                    insert_func(ds, perm(i));
                }
                mem = insert.memory_info();
            }
            result.log("memData", mem.current - mem.offset);
        }
        
        // predecessor queries
        {
            uint64_t chk = 0;
            {
                stat::Phase phase("predecessor_rnd");
                for(size_t i = 0; i < options.num_queries; i++) {
                    const uint64_t x = qperm_min + qperm(i);
                    auto r = pred_func(ds, x);
                    chk += r.pos;
                }
            }
            result.log("chk", chk);
        }

        // check
        if(options.check) {
            size_t num_errors = 0;
            for(size_t j = 0; j < options.num_queries; j++) {
                const uint64_t x = qperm_min + qperm(j);
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
            for(size_t i = initial_size; i < options.num; i++) {
                remove_func(ds, perm(i));
            }
        }
        
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
    cp.add_bytes('u', "universe", options.universe, "The size of the universe to draw from (default: 10 * n)");
    cp.add_bytes('q', "queries", options.num_queries, "The number to draw from the universe (default: 10M).");
    cp.add_bytes('s', "seed", options.seed, "The random seed.");
    cp.add_string("ds", options.ds, "The data structure to benchmark. If omitted, all data structures are benchmarked.");
    cp.add_flag("check", options.check, "Check results for correctness.");
    
    if(!cp.process(argc, argv)) {
        return -1;
    }

    if(!options.universe) {
        options.universe = 10 * options.num;
    } else {
        if(options.universe < options.num) {
            std::cerr << "universe not large enough" << std::endl;
            return -1;
        }
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

    bench("fusion_btree",
        [](const uint64_t){ return pred::dynamic::DynamicOctrie(); },
        [](const auto& ds){ return ds.size(); },
        [](auto& ds, const uint64_t x){ ds.insert(x); },
        [](const auto& ds, const uint64_t x){ return ds.predecessor(x); },
        [](auto& ds, const uint64_t x){ ds.remove(x); },
        perm, qperm, qmin);
    
    /*
    bench("index_hybrid",
        [](const uint64_t){ return pred::dynamic::DynIndex<tdc::pred::dynamic::bucket_hybrid, 16>(); },
        [](const auto& ds){ return ds.size(); },
        [](auto& ds, const uint64_t x){ ds.insert(x); },
        [](const auto& ds, const uint64_t x){ return ds.predecessor(x); },
        [](auto& ds, const uint64_t x){ ds.del(x); },
        perm, qperm, qmin);
        
    bench("index_bv",
        [](const uint64_t){ return pred::dynamic::DynIndex<tdc::pred::dynamic::bucket_bv, 16>(); },
        [](const auto& ds){ return ds.size(); },
        [](auto& ds, const uint64_t x){ ds.insert(x); },
        [](const auto& ds, const uint64_t x){ return ds.predecessor(x); },
        [](auto& ds, const uint64_t x){ ds.del(x); },
        perm, qperm, qmin);
        
    bench("index_list",
        [](const uint64_t){ return pred::dynamic::DynIndex<tdc::pred::dynamic::bucket_list, 16>(); },
        [](const auto& ds){ return ds.size(); },
        [](auto& ds, const uint64_t x){ ds.insert(x); },
        [](const auto& ds, const uint64_t x){ return ds.predecessor(x); },
        [](auto& ds, const uint64_t x){ ds.del(x); },
        perm, qperm, qmin);
    */
    
    bench("set",
        [](const uint64_t){ return std::set<uint64_t>(); },
        [](const auto& set){ return set.size(); },
        [](auto& set, const uint64_t x){ set.insert(x); },
        [](const auto& set, const uint64_t x){
            auto it = set.upper_bound(x);
            return pred::Result { it != set.begin(), *(--it) };
        },
        [](auto& set, const uint64_t x){ set.erase(x); },
        perm, qperm, qmin);
        
#ifdef PLADS_FOUND
    bench("dbv",
        [](const uint64_t){ return pred::dynamic::DynamicRankSelect(); },
        [](const auto& dbv){ return dbv.size(); },
        [](auto& dbv, const uint64_t x){ dbv.insert(x); },
        [](const auto& dbv, const uint64_t x){ return dbv.predecessor(x); },
        [](auto& dbv, const uint64_t x){ dbv.remove(x); },
        perm, qperm, qmin);
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
            [](auto& stree, const uint64_t x){ stree.del(x); },
            perm, qperm, qmin);
    } else {
        std::cerr << "WARNING: STree only supports 31-bit universes and will therefore not be benchmarked" << std::endl;
    }
#endif

    return 0;
}
    
