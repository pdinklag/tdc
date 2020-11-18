#include <cassert>
#include <iostream>
#include <limits>
#include <random>
#include <set>

#include <tdc/math/ilog2.hpp>
#include <tdc/pred/dynamic/btree.hpp>
#include <tdc/pred/dynamic/btree/sorted_array_node.hpp>
#include <tdc/stat/time.hpp>
#include <tdc/util/benchmark/integer_operation.hpp>

#include <tdc/mpf/random/engine.hpp>
#include <tdc/mpf/random/normal_distribution.hpp>
#include <tdc/mpf/random/uniform_distribution.hpp>

#include <tlx/cmdline_parser.hpp>

/*
 * Binary output format specification:
 *
 * uint64_t u     -- universe is 2^u-1
 * uint64_t batch -- batch size
 * sequence of:
 * {
 *     char  opcode      -- operation code
 *     key_t keys[batch] -- operation keys
 * }
 *
 * ###
 * key_t is determined by u:
 *      u <=  64 -> key_t =  uint64_t
 *      u <= 128 -> key_t = uint128_t
 *      u <= 256 -> key_t = uint256_t
 *      etc
 */

using namespace tdc;

struct {
    size_t max_num = 100ULL;
    size_t max_ops = SIZE_MAX;
    uint64_t universe = 32;
    size_t key_seed = stat::time_nanos();
    size_t op_seed = stat::time_nanos();
    size_t batch = 10ULL;
    double p_base = 0.3;
    double p_range = 0.5;
    double p_query = 0.9;
    double hold = 0.25;
    std::string distr = "uniform";
    uint64_t n_mean = 2;
    uint64_t n_stddev = 8;
    bool readable = false;
    bool print_opnum = false;
    bool print_num = false;
} options;

template<typename key_t>
void generate() {
    constexpr key_t KEY_MAX = std::numeric_limits<key_t>::max();
    constexpr size_t KEY_BITS = std::numeric_limits<key_t>::digits;
    
    const key_t u = KEY_MAX >> (KEY_BITS - options.universe);
    
    // random generator
    std::mt19937 gen_op(options.op_seed);
    std::uniform_real_distribution<double> random_op(0.0, 1.0);
    
    mpf::random::Engine gen_val(options.key_seed);
    mpf::random::UniformDistribution<KEY_BITS> random_from_universe(0ULL, u);
    
    // working set
    pred::dynamic::BTree<key_t, 65, pred::dynamic::SortedArrayNode<key_t, 64>> cur_set;
    key_t* cur_arr = new key_t[options.max_num];
    
    size_t cur_num = 0;
    key_t cur_min = KEY_MAX;
    key_t cur_max = 0;
    
    // operations
    struct op_t {
        char code;
        key_t x;
    } __attribute__((packed));
    static_assert(sizeof(op_t) == 9);
    
    size_t count_insert = 0;
    size_t count_delete = 0;
    size_t count_query = 0;
    size_t failed_inserts = 0;
    
    auto generate_insert = [&](){
        ++count_insert;
        
        key_t x = (key_t)random_from_universe(gen_val);
        while(cur_set.contains(x)) {
            x = (key_t)random_from_universe(gen_val);
            ++failed_inserts;
        }
        
        cur_set.insert(x);

        assert(cur_num < options.max_num);
        cur_arr[cur_num] = x;
        
        ++cur_num;
        cur_min = std::min(cur_min, x);
        cur_max = std::max(cur_max, x);
        
        return x;
    };
    
    auto generate_query = [&](){
        assert(cur_num > 0);
        ++count_query;
        
        mpf::random::UniformDistribution<KEY_BITS> random_range(cur_min, cur_max);
        return (key_t)random_range(gen_val);
    };
    
    auto generate_delete = [&](){
        assert(cur_num > 0);
        ++count_delete;
        
        mpf::random::UniformDistribution<KEY_BITS> random_index(0, cur_num - 1);
        const size_t i = (size_t)random_index(gen_val);
        const key_t x = cur_arr[i];
        
        cur_set.remove(x);
        cur_arr[i] = cur_arr[cur_num - 1]; // "delete" = swap with last, order isn't important
        
        --cur_num;
        if(cur_num > 0) {
            if(x == cur_min) cur_min = cur_set.min();
            if(x == cur_max) cur_max = cur_set.max();
        } else {
            cur_min = KEY_MAX;
            cur_max = 0;
        }

        return x ;
    };
    
    size_t count_total = 0;

    auto output_opcode = [&](char op){
        if(options.readable) {
            switch(op) {
                case benchmark::OPCODE_INSERT:
                    std::cout << "INSERT:" << std::endl;
                    break;

                case benchmark::OPCODE_QUERY:
                    std::cout << "QUERY:" << std::endl;
                    break;

                case benchmark::OPCODE_DELETE:
                    std::cout << "DELETE:" << std::endl;
                    break;

                default:
                    std::abort();
                    break;
            }
        } else {
            std::cout << op;
        }
    };
    
    auto output_key = [&](const key_t& key){
        ++count_total;

        // print
        if(options.readable) {
            std::cout << '\t';
            if(options.print_opnum) std::cout << count_total << '\t';
            std::cout << key;
            if(options.print_num) std::cout << '\t' << cur_num;
            std::cout << std::endl;
        } else {
            std::cout.write((const char*)&key, sizeof(key));
        }
    };

    auto output_insert_batch = [&](){
        output_opcode(benchmark::OPCODE_INSERT);
        for(size_t i = 0; i < options.batch; i++) {
            output_key(generate_insert());
        }
    };

    auto output_query_batch = [&](){
        output_opcode(benchmark::OPCODE_QUERY);
        for(size_t i = 0; i < options.batch; i++) {
            output_key(generate_query());
        }
    };

    auto output_delete_batch = [&](){
        output_opcode(benchmark::OPCODE_DELETE);
        for(size_t i = 0; i < options.batch; i++) {
            output_key(generate_delete());
        }
    };
    
    auto insert_probability = [&](){
        return options.p_base + options.p_range * double(options.max_num - cur_num) / double(options.max_num);
    };
    
    // insertion phase
    while(cur_num + options.batch < options.max_num && count_total < options.max_ops * options.batch) {
        if(cur_num < options.batch || random_op(gen_op) <= insert_probability()) {
            output_insert_batch();
            continue;
        }
        
        if(cur_num > 0 && random_op(gen_op) <= options.p_query) {
            output_query_batch();
            continue;
        }
        
        else {
            output_delete_batch();
        }
    }

    // hold phase
    const size_t max_hold_ops = size_t(options.hold * double(count_total));
    for(size_t i = 0; i < max_hold_ops && count_total < options.max_ops * options.batch; i++) {
        const float p = random_op(gen_op);
        
        if(cur_num + options.batch < options.max_num) {
            if(cur_num < options.batch || p <= (1.0/3.0)) {
                output_insert_batch();
            } else if(p <= (2.0/3.0)) {
                output_delete_batch();
            } else {
                output_query_batch();
            }
        } else {
            if(cur_num >= options.batch && p <= 0.5) {
                output_delete_batch();
            } else {
                output_query_batch();
            }
        }
    }

    // deletion phase
    while(cur_num > 0 && count_total < options.max_ops * options.batch) {
        if(cur_num + options.batch >= options.max_num || random_op(gen_op) <= insert_probability()) {
            output_delete_batch();
            continue;
        }
        
        if(cur_num > 0 && random_op(gen_op) <= options.p_query) {
            output_query_batch();
            continue;
        }
        
        else if(cur_num + options.batch < options.max_num) {
            output_insert_batch();
        }
    }
    
    std::cerr << "generated "
        << count_total << " operations (key seed: "
        << options.key_seed << ", op seed: "
        << options.op_seed << ", "
        << failed_inserts << " duplicates prevented): "
        << count_insert << " inserts, "
        << count_delete << " deletes and "
        << count_query << " queries"
        << std::endl;

    // clean up
    delete[] cur_arr;
}

int main(int argc, char** argv) {
    tlx::CmdlineParser cp;
    cp.set_description("Generates a sequence of inserts, queries and deletes to simulate operation of a dynamic data structure.");
    cp.add_bytes('n', "num", options.max_num, "The (lenient) maximum number of items in the data structure (default: 100).");
    cp.add_bytes('m', "max-ops", options.max_ops, "The maximum number of operations to generate (default: infinite).");
    cp.add_bytes('u', "universe", options.universe, "The base-2 logarithm of the universe to draw numbers from (default: 32).");
    cp.add_size_t('s', "key-seed", options.key_seed, "The seed for random key generation (default: timestamp).");
    cp.add_size_t('t', "op-seed", options.op_seed, "The seed for random operation generation (default: timestamp).");
    cp.add_bytes('b', "batch", options.batch, "The batch size for operations.");
    cp.add_double('p', "p-base", options.p_base, "The base probability for inserts/deletes in the corresponding phase (default: 0.3)");
    cp.add_double('r', "p-range", options.p_range, "The probability range for inserts/deletes in the corresponding phase (default: 0.5)");
    cp.add_double('q', "p-query", options.p_query, "The probability for queries, if not the phase's primary operation (default: 0.9)");
    cp.add_string('d', "distribution", options.distr, "The distribution of inserted keys in the universe -- 'uniform' or 'normal' (default: uniform)");
    cp.add_size_t("n-mean", options.n_mean, "The mean for a normal distribution will be U/n_mean (default: 2).");
    cp.add_size_t("n-stddev", options.n_mean, "The standard deviation for a normal distribution will be U/n_stddev (default: 8).");
    cp.add_double("hold", options.hold, "The duration of the hold phase, relative to the duration of the insertion phase (default: 0.25)");
    cp.add_flag("readable", options.readable, "Create a human-readable output rather than a binary file.");
    cp.add_flag("print-opnum", options.print_opnum, "Print the number of each operation.");
    cp.add_flag("print-num", options.print_num, "Print the number of items after each operation.");

    if(!cp.process(argc, argv)) {
        return -1;
    }

    if(options.universe < math::ilog2_ceil(options.max_num)) {
        std::cerr << "the universe must be at least as large as the maximum number of items" << std::endl;
        return -2;
    }
    
    if(options.p_base + options.p_range >= 1) {
        std::cerr << "p_base + p_range must be less than one" << std::endl;
        return -3;
    }
    
    if(options.p_query >= 1) {
        std::cerr << "p_query must be less than one" << std::endl;
        return -4;
    }

    // output header
    if(options.readable) {
        std::cout << "u=" << options.universe << std::endl;
    } else {
        std::cout.write((const char*)&options.universe, sizeof(options.universe));
        std::cout.write((const char*)&options.batch, sizeof(options.batch));
    }

    // TODO: select distribution

    if(options.universe <= 64) {
        generate<uint64_t>();
    } else {
        std::cerr << "universe size not supported" << std::endl;
        return -5;
    }
}
