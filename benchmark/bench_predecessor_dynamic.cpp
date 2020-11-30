#include <fstream>
#include <iostream>
#include <set>
#include <vector>

#include <tdc/io/mmap_file.hpp>
#include <tdc/math/ilog2.hpp>
#include <tdc/random/permutation.hpp>
#include <tdc/random/vector.hpp>
#include <tdc/stat/phase.hpp>
#include <tdc/stat/time.hpp>
#include <tdc/rapl/rapl_phase_extension.hpp>
#include <tdc/uint/uint40.hpp>
#include <tdc/uint/uint256.hpp>

#include <tdc/pred/binary_search.hpp>
#include <tdc/pred/dynamic/dynamic_index.hpp>
#include <tdc/pred/dynamic/dynamic_index_map.hpp>
#include <tdc/pred/dynamic/dynamic_pred_bv.hpp>
#include <tdc/pred/dynamic/dynamic_rankselect.hpp>
#include <tdc/pred/dynamic/yfast.hpp>

#include <tdc/pred/dynamic/btree.hpp>
#include <tdc/pred/dynamic/btree/dynamic_fusion_node.hpp>
#include <tdc/pred/dynamic/btree/sorted_array_node.hpp>

#include <tdc/util/literals.hpp>
#include <tdc/util/benchmark/integer_operation.hpp>

#include <tlx/cmdline_parser.hpp>

#ifdef INTEGERSTRUCTURES_FOUND
    #define BENCH_BURST
    #include <btrie/lpcbtrie.h>
    
    // wrapper around lpcbtrie
    // - adds a size field
    // - implements predecessor rather than successor by negating keys
    // - converts uint40_t key type to uint64_t in the index type because LPCBTrie cannot handle uint40_t due to implicit conversions
    template<typename key_t>
    class LPCBTrieWrapper {
    private:
        using real_key_t = typename std::conditional<std::is_same<key_t, uint40_t>::value, uint64_t, key_t>::type;
        
        mutable LPCBTrie<real_key_t, key_t> m_trie;
        size_t m_size;
        
    public:
        LPCBTrieWrapper() : m_size(0) {
        }
        
        void insert(const key_t& x) {
            m_trie.insert(~(real_key_t)x, x);
            ++m_size;
        }
        
        tdc::pred::KeyResult<key_t> pred(const key_t& x) const {
            key_t* result = m_trie.locate(~(real_key_t)x);
            return tdc::pred::KeyResult<key_t> { result != nullptr, result ? *result : 0 };
        }
        
        void remove(const key_t& x) {
            #ifndef NDEBUG
            // we can't remove non-existing keys here
            auto r = pred(x);
            assert(r && r.key == x);
            #endif
            
            m_trie.remove(~(real_key_t)x);
            --m_size;
        }
        
        size_t size() const {
            return m_size;
        }
    };
#endif

#if defined(LEDA_FOUND) && defined(STREE_FOUND)
    #define BENCH_STREE
    #include <veb/STree_orig.h>
#endif

using namespace tdc;

constexpr size_t OPS_READ_BUFSIZE = 64_Mi;

struct {
    size_t num = 1_M;
    uint64_t universe = 0;
    size_t num_queries = 1_M;
    uint64_t seed = random::DEFAULT_SEED;
    
    std::string ds; // if non-empty, only benchmarks the selected data structure
    
    std::string ops_filename = "";
    std::ifstream ops;
    std::ifstream::pos_type ops_rewind_pos;
    
    bool has_opsfile() const {
        return ops_filename.length() > 0;
    }
    
    void rewind_ops() {
        ops = std::ifstream(ops_filename);
        ops.seekg(ops_rewind_pos, std::ios::beg);
    }
    
    bool do_bench(const std::string& name) const {
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
template<typename key_t, typename ctor_func_t, typename size_func_t, typename insert_func_t, typename pred_func_t, typename remove_func_t>
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
    
    if(options.num > 0) {
        // input
        result.log("num", options.num);
        result.log("universe", options.universe);
        result.log("queries", options.num_queries);
        result.log("seed", options.seed);
        
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
                    chk_q += (uint64_t)pred_func(ds, options.perm_queries(i)).key;
                }
            }
            result.log("chk", chk_q);
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
                if(r.key == options.data[correct_result.pos]) {
                    // OK
                } else {
                    // nah, count an error
                    //std::cout << std::hex << "index: " << x << "  correct: " << options.data[correct_result.pos] << "  wrong: " << r.key << std::endl;
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
            result.log("memEmpty", mem.current);
        }
    }
    
    if(options.has_opsfile() > 0) {
        result.log("ops", options.ops_filename);
        
        // init data structure
        auto ds = ctor_func(0);
        if(size_func(ds) == 1) remove_func(ds, 0);
        
        assert(size_func(ds) == 0);
        
        uint64_t ops_chk = 0;
        size_t ops_total = 0;
        size_t ops_ins = 0;
        size_t ops_del = 0;
        size_t ops_q = 0;
        size_t ops_max = 0;
        uint64_t time_ins = 0;
        uint64_t time_del = 0;
        uint64_t time_q = 0;
        {
            options.rewind_ops();
            stat::Phase ops_phase("ops");
            
            benchmark::IntegerOperationBatch<key_t> batch;
            while(batch.read(options.ops)) {
                // process next batch
                ops_total += batch.size();
                
                uint64_t t0;
                switch(batch.opcode()) {
                    case benchmark::OPCODE_INSERT:
                        t0 = stat::time_nanos();
                        for(const auto& key : batch.keys()) {
                            insert_func(ds, key);
                        }
                        time_ins += stat::time_nanos() - t0;
                        ops_ins += batch.size();
                        ops_max = std::max(ops_max, (size_t)size_func(ds));
                        break;
                        
                    case benchmark::OPCODE_DELETE:
                        t0 = stat::time_nanos();
                        for(const auto& key : batch.keys()) {
                            remove_func(ds, key);
                        }
                        time_del += stat::time_nanos() - t0;
                        ops_del += batch.size();
                        break;
                        
                    case benchmark::OPCODE_QUERY:
                        t0 = stat::time_nanos();
                        for(const auto& key : batch.keys()) {
                            ops_chk += (uint64_t)pred_func(ds, key).key;
                        }
                        time_q += stat::time_nanos() - t0;
                        ops_q += batch.size();
                        break;
                }
            }
            
            auto mem = ops_phase.memory_info();
            result.log("memPeak_ops", mem.peak);
        }
        
        result.log("ops_total", ops_total);
        result.log("ops_ins", ops_ins);
        result.log("ops_del", ops_del);
        result.log("ops_q", ops_q);
        result.log("ops_chk", ops_chk);
        result.log("ops_max", ops_max);
        result.log("time_ins", (double)(time_ins / 1000ULL) / 1000.0);
        result.log("time_del", (double)(time_del / 1000ULL) / 1000.0);
        result.log("time_q", (double)(time_q / 1000ULL) / 1000.0);
    }
    
    std::cout << "RESULT algo=" << name << " " << result.to_keyval() << " " << result.subphases_keyval() << std::endl;
}

template<typename key_t>
void benchmark_arbitrary_universe() {
    bench<key_t>("fusion_btree_8",
        [](const key_t){ return pred::dynamic::BTree<key_t, 9, pred::dynamic::DynamicFusionNode<key_t, 8>>(); },
        [](const auto& ds){ return ds.size(); },
        [](auto& ds, const key_t x){ ds.insert(x); },
        [](const auto& ds, const key_t x){ return ds.predecessor(x); },
        [](auto& ds, const key_t x){ ds.remove(x); }
    );
    bench<key_t>("fusion_btree_16",
        [](const key_t){ return pred::dynamic::BTree<key_t, 17, pred::dynamic::DynamicFusionNode<key_t, 16>>(); },
        [](const auto& ds){ return ds.size(); },
        [](auto& ds, const key_t x){ ds.insert(x); },
        [](const auto& ds, const key_t x){ return ds.predecessor(x); },
        [](auto& ds, const key_t x){ ds.remove(x); }
    );
    bench<key_t>("btree_8",
        [](const key_t){ return pred::dynamic::BTree<key_t, 9, pred::dynamic::SortedArrayNode<key_t, 8>>(); },
        [](const auto& ds){ return ds.size(); },
        [](auto& ds, const key_t x){ ds.insert(x); },
        [](const auto& ds, const key_t x){ return ds.predecessor(x); },
        [](auto& ds, const key_t x){ ds.remove(x); }
    );
    bench<key_t>("btree_64",
        [](const key_t){ return pred::dynamic::BTree<key_t, 65, pred::dynamic::SortedArrayNode<key_t, 64>>(); },
        [](const auto& ds){ return ds.size(); },
        [](auto& ds, const key_t x){ ds.insert(x); },
        [](const auto& ds, const key_t x){ return ds.predecessor(x); },
        [](auto& ds, const key_t x){ ds.remove(x); }
    );
}

template<typename key_t>
void benchmark_large_universe() {
    benchmark_arbitrary_universe<key_t>();
    bench<key_t>("yfast_trie-06",
        [](const key_t){ return pred::dynamic::YFastTrie<key_t, std::numeric_limits<key_t>::digits, 6>(); },
        [](const auto& ds){ return ds.size(); },
        [](auto& ds, const key_t x){ ds.insert((uint64_t)x); },
        [](const auto& ds, const key_t x){ return ds.predecessor((uint64_t)x); },
        [](auto& ds, const key_t x){ ds.remove((uint64_t)x); }
    );
    bench<key_t>("yfast_trie-07",
        [](const key_t){ return pred::dynamic::YFastTrie<key_t, std::numeric_limits<key_t>::digits, 7>(); },
        [](const auto& ds){ return ds.size(); },
        [](auto& ds, const key_t x){ ds.insert((uint64_t)x); },
        [](const auto& ds, const key_t x){ return ds.predecessor((uint64_t)x); },
        [](auto& ds, const key_t x){ ds.remove((uint64_t)x); }
    );
    bench<key_t>("yfast_trie-08",
        [](const key_t){ return pred::dynamic::YFastTrie<key_t, std::numeric_limits<key_t>::digits, 8>(); },
        [](const auto& ds){ return ds.size(); },
        [](auto& ds, const key_t x){ ds.insert((uint64_t)x); },
        [](const auto& ds, const key_t x){ return ds.predecessor((uint64_t)x); },
        [](auto& ds, const key_t x){ ds.remove((uint64_t)x); }
    );
    bench<key_t>("yfast_trie-09",
        [](const key_t){ return pred::dynamic::YFastTrie<key_t, std::numeric_limits<key_t>::digits, 9>(); },
        [](const auto& ds){ return ds.size(); },
        [](auto& ds, const key_t x){ ds.insert((uint64_t)x); },
        [](const auto& ds, const key_t x){ return ds.predecessor((uint64_t)x); },
        [](auto& ds, const key_t x){ ds.remove((uint64_t)x); }
    );
#ifdef BENCH_BURST
    bench<key_t>("burst_trie",
        [](const key_t){ return LPCBTrieWrapper<key_t>(); },
        [](const auto& trie){ return trie.size(); },
        [](auto& trie, const key_t x){ trie.insert(x); },
        [](const auto& trie, const key_t x){ return trie.pred(x); },
        [](auto& trie, const key_t x){ trie.remove(x); }
    );
#endif
}

template<typename key_t>
void benchmark_medium_universe() {
    benchmark_large_universe<key_t>();
    
    bench<key_t>("index_hybrid",
        [](const key_t){ return pred::dynamic::DynIndex<tdc::pred::dynamic::bucket_hybrid, 16>(); },
        [](const auto& ds){ return ds.size(); },
        [](auto& ds, const key_t x){ ds.insert((uint64_t)x); },
        [](const auto& ds, const key_t x){ return ds.predecessor((uint64_t)x); },
        [](auto& ds, const key_t x){ ds.remove((uint64_t)x); }
    );
    bench<key_t>("map_hybrid",
        [](const key_t){ return pred::dynamic::DynIndexMap<tdc::pred::dynamic::map_bucket_hybrid, 16>(); },
        [](const auto& ds){ return ds.size(); },
        [](auto& ds, const key_t x){ ds.insert((uint64_t)x); },
        [](const auto& ds, const key_t x){ return ds.predecessor((uint64_t)x); },
        [](auto& ds, const key_t x){ ds.remove((uint64_t)x); }
    );
}

template<typename key_t>
void benchmark_small_universe() {
    benchmark_medium_universe<key_t>();
    
#ifdef BENCH_STREE
    if(options.universe < 32) {
        bench<key_t>("stree",
            [](const key_t first){
                return STree_orig<>(options.universe, first); // STree cannot be empty
            },
            [](auto& stree){ return stree.getSize(); },
            [](auto& stree, const key_t x){ stree.insert((uint32_t)x); },
            [](auto& stree, const key_t x){
                // STree seems to look for the largest value STRICTLY LESS THAN the input
                // and crashes if there is no predecessor...
                return pred::KeyResult<key_t> { true, (key_t)stree.pred((uint32_t)(x+1)) };
            },
            [](auto& stree, const key_t x){ stree.del((uint32_t)x); }
        );
    }
#endif
}

int main(int argc, char** argv) {
#ifdef TDC_RAPL_AVAILABLE
    stat::Phase::register_extension<rapl::RAPLPhaseExtension>();
#endif
    
    tlx::CmdlineParser cp;
    cp.add_bytes('n', "num", options.num, "The length of the sequence (default: 1M).");
    cp.add_bytes('u', "universe", options.universe, "The base-2 logarithm of the universe to draw from (default: 2x num)");
    cp.add_bytes('q', "queries", options.num_queries, "The number to draw from the universe (default: 1M).");
    cp.add_bytes('s', "seed", options.seed, "The random seed.");
    cp.add_string("ds", options.ds, "The data structure to benchmark. If omitted, all data structures are benchmarked.");
    cp.add_string("ops", options.ops_filename, "The file containing the operation sequence to benchmark, if any.");
    cp.add_flag("check", options.check, "Check results for correctness.");
    
    if(!cp.process(argc, argv)) {
        return -1;
    }

    if(options.has_opsfile()) {
        // process ops only
        options.num = 0;
        
        // open file and read universe
        options.ops = std::ifstream(options.ops_filename);
        options.ops.read((char*)&options.universe, sizeof(options.universe));
        options.ops_rewind_pos = options.ops.tellg();
    }
    else if(options.num > 0) {
        if(!options.universe) {
            std::cerr << "universe required" << std::endl;
            return -1;
        } else if(options.universe > 64) {
            std::cerr << "base benchmark currently only supports universes up to 64 bits" << std::endl;
            return -1;
        }
        
        const uint64_t u = UINT64_MAX >> (64 - options.universe);
        if(u < options.num + 1) {
            std::cerr << "universe not large enough" << std::endl;
            return -1;
        }
        
        // generate permutation
        // we subtract 1 from the universe because we add it back for the insertions
        options.perm_values = random::Permutation(u - 1, options.seed);

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
        
        options.perm_queries = random::Permutation(u, options.seed ^ 0x1234ABCD);
    } else {
        std::cout << "nothing to do!" << std::endl;
        return 0;
    }
    
    if(options.universe <= 32) {
        benchmark_small_universe<uint32_t>();
    } else if(options.universe <= 40) {
        benchmark_medium_universe<uint40_t>();
    } else if(options.universe <= 64) {
        benchmark_large_universe<uint64_t>();
    } else if(options.universe <= 128) {
        benchmark_arbitrary_universe<uint128_t>();
    }
    
    return 0;
}
    
