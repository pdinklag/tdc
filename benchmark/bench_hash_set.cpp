#include <cassert>
#include <iostream>
#include <unordered_set>

#include <tdc/hash/set.hpp>

#include <tdc/hash/linear_probing.hpp>
#include <tdc/hash/quadratic_probing.hpp>

#include <tdc/random/permutation.hpp>
#include <tdc/stat/phase.hpp>

#include <tlx/cmdline_parser.hpp>

using namespace tdc;

struct {
    size_t capacity = 0;
    double load_factor = 1.0;
    double growth_factor = 2.0;
    size_t num = 1'000;
    size_t num_queries = 100'000;
    uint64_t universe = UINT32_MAX;
    uint64_t seed = random::DEFAULT_SEED;
    std::string ds = "";

    random::Permutation keys;
    random::Permutation queries;
} options;

stat::Phase benchmark_phase(std::string&& title) {
    stat::Phase phase(std::move(title));
    phase.log("num", options.num);
    phase.log("universe", options.universe);
    phase.log("queries", options.num_queries);
    phase.log("seed", options.seed);
    phase.log("capacity", options.capacity);
    phase.log("load_factor", options.load_factor);
    phase.log("growth_factor", options.growth_factor);
    return phase;
}

template<typename ctor_t, typename diag_func_t>
void bench(const std::string& name, ctor_t ctor, diag_func_t diag) {
    if(options.ds.length() > 0 && options.ds != name) return;
    
    auto result = benchmark_phase("");
    using set_t = decltype(ctor());
    set_t set;

    // insert
    {
        stat::Phase::MemoryInfo mem;
        {
            stat::Phase phase("insert");
            set = ctor();
            for(size_t i = 0; i < options.num; i++) {
                auto key = options.keys(i);
                set.insert(key);
            }
            mem = phase.memory_info();
        }

        assert(set.size() == options.num);

        result.log("memData", mem.current - mem.offset);
        diag(result, set);
    }

    // lookup existing keys
    {
        stat::Phase phase("lookup_existing");
        for(size_t i = 0; i < options.num; i++) {
            auto key = options.keys(i);
            auto x = set.find(key);

            assert(x != set.end());
        }
    }

    // lookup random keys
    {
        size_t chk = 0;
        {
            stat::Phase phase("lookup_rnd");
            for(size_t i = 0; i < options.num_queries; i++) {
                auto key = options.queries(i);
                auto x = set.find(key);

                chk += (x != set.end());
            }
        }
        result.log("chk", chk);
    }

    // output
    std::cout << "RESULT algo=" << name << " " << result.to_keyval() << " " << result.subphases_keyval() << std::endl;
}

size_t id(uint64_t key) {
    return key;
}

size_t mix(uint64_t key) {
    key = (~key) + (key << 21ULL);
    key = key ^ (key >> 24ULL);
    key = (key + (key << 3ULL)) + (key << 8ULL);
    key = key ^ (key >> 14ULL);
    key = (key + (key << 2ULL)) + (key << 4ULL);
    key = key ^ (key >> 28ULL);
    key = key + (key << 31ULL);
    return key;
}

struct mul_hash {
    uint64_t prime;

    inline size_t operator()(uint64_t key) {
        return key * prime;
    }
};

void diag_tdc(stat::Phase& result, const hash::Set<uint64_t>& set) {
    result.log("load", set.load());
    result.log("max_probe", set.max_probe());
    
#ifndef NDEBUG
    result.log("avg_probe", set.avg_probe());
    result.log("times_resized", set.times_resized());
#endif
}

template<typename hash_func_t, typename probe_func_t>
void bench_tdc(const std::string& name, hash_func_t hash_func, probe_func_t probe_func) {
    bench(name, [&](){ return hash::Set<uint64_t>(hash_func, options.capacity, options.load_factor, options.growth_factor, probe_func); }, diag_tdc);
}

int main(int argc, char** argv) {
    tlx::CmdlineParser cp;

    cp.add_bytes('u', "universe", options.universe, "the universe to draw hashtable entries from (default: 32 bit numbers)");
    cp.add_bytes('n', "num", options.num, "the number of hashtable entries to draw (default: 1024)");
    cp.add_bytes('c', "capacity", options.capacity, "the initial capacity (default: input size)");
    cp.add_double('l', "load-factor", options.load_factor, "the maximum load factor (default: 1)");
    cp.add_double('g', "growth-factor", options.growth_factor, "the growth factor (default: 2)");
    cp.add_bytes('q', "queries", options.num_queries, "the number of membership queries to perform");
    cp.add_bytes('s', "seed", options.seed, "the random seed");
    cp.add_string("ds", options.ds, "the single data structure to test");
    
    if (!cp.process(argc, argv)) {
        return -1;
    }

    options.keys    = random::Permutation(options.universe, options.seed);
    options.queries = random::Permutation(options.universe, options.seed + 1);

    if(options.capacity == 0) {
        options.capacity = options.num;
    }

    // unordered set
    bench("std::unordered_set", [](){ return std::unordered_set<uint64_t>(options.capacity); }, [](stat::Phase&, const std::unordered_set<uint64_t>&){});

    bench_tdc("lp.id",         id,                                      hash::LinearProbing<>{});
    bench_tdc("lp.knuth",      mul_hash{2654435761ULL},                 hash::LinearProbing<>{});
    bench_tdc("lp.mul_prime1", mul_hash{15'425'459'083'914'370'367ULL}, hash::LinearProbing<>{});
    bench_tdc("lp.mul_prime2", mul_hash{16'568'458'216'213'224'001ULL}, hash::LinearProbing<>{});
    bench_tdc("lp.mul_prime3", mul_hash{17'406'548'584'874'384'839ULL}, hash::LinearProbing<>{});
    bench_tdc("lp.mix",        mix,                                     hash::LinearProbing<>{});
    
    bench_tdc("qp.id",         id,                                      hash::QuadraticProbing<>{});
    bench_tdc("qp.knuth",      mul_hash{2654435761ULL},                 hash::QuadraticProbing<>{});
    bench_tdc("qp.mul_prime1", mul_hash{15'425'459'083'914'370'367ULL}, hash::QuadraticProbing<>{});
    bench_tdc("qp.mul_prime2", mul_hash{16'568'458'216'213'224'001ULL}, hash::QuadraticProbing<>{});
    bench_tdc("qp.mul_prime3", mul_hash{17'406'548'584'874'384'839ULL}, hash::QuadraticProbing<>{});
    bench_tdc("qp.mix",        mix,                                     hash::QuadraticProbing<>{});
    
    return 0;
}
