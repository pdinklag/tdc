#include <cassert>
#include <fstream>
#include <iostream>
#include <limits>
#include <random>
#include <set>

#include <tdc/math/ilog2.hpp>
#include <tdc/pred/dynamic/btree.hpp>
#include <tdc/pred/dynamic/btree/sorted_array_node.hpp>
#include <tdc/stat/time.hpp>
#include <tdc/uint/uint40.hpp>
#include <tdc/uint/uint256.hpp>
#include <tdc/util/benchmark/integer_operation.hpp>

#include <tdc/mpf/random/engine.hpp>
#include <tdc/mpf/random/normal_distribution.hpp>
#include <tdc/mpf/random/uniform_distribution.hpp>

#include <tlx/cmdline_parser.hpp>

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
    
    std::string out_filename = "";
    
    bool print_opnum = false;
    bool print_num = false;
    
    std::string csv_filename = "";
    inline bool csv() const {
        return csv_filename.length() > 0;
    }
} options;

struct {
    size_t count_insert = 0;
    size_t count_delete = 0;
    size_t count_query = 0;
    size_t failed_inserts = 0;
    
    inline size_t count_total() const {
        return count_insert + count_delete + count_query;
    }
} stats;

template<typename key_t, typename key_generator_t>
benchmark::IntegerOperationBatch<key_t> generate_batch(const benchmark::opcode_t opcode, key_generator_t gen) {
    benchmark::IntegerOperationBatch<key_t> batch(opcode, options.batch);
    for(size_t i = 0; i < options.batch; i++) {
        batch.add_key(gen());
    }
    return batch;
}

template<typename key_t>
void output_batch(std::ostream& out, const benchmark::IntegerOperationBatch<key_t>& batch) {
    batch.write(out);
}

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
    
    // out file
    std::ofstream out;
    out = std::ofstream(options.out_filename);
    out.write((const char*)&options.universe, sizeof(options.universe));
    
    // csv file
    std::ofstream csv;
    if(options.csv()) {
        csv = std::ofstream(options.csv_filename);
        csv << "ops,keys" << std::endl;
    }
    
    // working set
    pred::dynamic::BTree<key_t, 65, pred::dynamic::SortedArrayNode<key_t, 64>> cur_set;
    key_t* cur_arr = new key_t[options.max_num];
    
    size_t cur_num = 0;
    key_t cur_min = KEY_MAX;
    key_t cur_max = 0;
    
    // operations
    auto generate_insert = [&](){
        ++stats.count_insert;
        
        key_t x = (key_t)random_from_universe(gen_val);
        while(cur_set.contains(x)) {
            x = (key_t)random_from_universe(gen_val);
            ++stats.failed_inserts;
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
        ++stats.count_query;
        
        mpf::random::UniformDistribution<KEY_BITS> random_range(cur_min, cur_max);
        return (key_t)random_range(gen_val);
    };
    
    auto generate_delete = [&](){
        assert(cur_num > 0);
        ++stats.count_delete;
        
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
    
    auto generate_insert_batch = [&](){
        return generate_batch<key_t>(benchmark::OPCODE_INSERT, generate_insert);
    };

    auto generate_query_batch = [&](){
        return generate_batch<key_t>(benchmark::OPCODE_QUERY, generate_query);
    };

    auto generate_delete_batch = [&](){
        return generate_batch<key_t>(benchmark::OPCODE_DELETE, generate_delete);
    };
    
    auto generate_and_output = [&](const benchmark::opcode_t opcode){
        switch(opcode) {
            case benchmark::OPCODE_INSERT: output_batch(out, generate_insert_batch()); break;
            case benchmark::OPCODE_QUERY: output_batch(out, generate_query_batch()); break;
            case benchmark::OPCODE_DELETE: output_batch(out, generate_delete_batch()); break;
            default: std::abort(); break;
        }
        
        if(options.csv()) {
            csv << stats.count_total() << "," << cur_num << std::endl;
        }
    };
    
    auto output_insert_batch = [&](){
        generate_and_output(benchmark::OPCODE_INSERT);
    };

    auto output_query_batch = [&](){
        generate_and_output(benchmark::OPCODE_QUERY);
    };

    auto output_delete_batch = [&](){
        generate_and_output(benchmark::OPCODE_DELETE);
    };
    
    auto insert_probability = [&](){
        return options.p_base + options.p_range * double(options.max_num - cur_num) / double(options.max_num);
    };
    
    // insertion phase
    while(cur_num + options.batch < options.max_num && stats.count_total() < options.max_ops * options.batch) {
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
    const size_t max_hold_ops = size_t(options.hold * double(stats.count_total() / options.batch));
    for(size_t i = 0; i < max_hold_ops && stats.count_total() < options.max_ops * options.batch; i++) {
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
    while(cur_num > 0 && stats.count_total() < options.max_ops * options.batch) {
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
        << stats.count_total() << " operations (key seed: "
        << options.key_seed << ", op seed: "
        << options.op_seed << ", "
        << stats.failed_inserts << " duplicates prevented): "
        << stats.count_insert << " inserts, "
        << stats.count_delete << " deletes and "
        << stats.count_query << " queries"
        << std::endl;

    // clean up
    delete[] cur_arr;
}

int main(int argc, char** argv) {
    tlx::CmdlineParser cp;
    cp.set_description("Generates a sequence of inserts, queries and deletes to simulate operation of a dynamic data structure");
    cp.add_param_string("out", options.out_filename, "Write the output to the specified file");
    cp.add_bytes('n', "num", options.max_num, "The (lenient) maximum number of items in the data structure (default: 100)");
    cp.add_bytes('m', "max-ops", options.max_ops, "The maximum number of operations to generate (default: infinite)");
    cp.add_bytes('u', "universe", options.universe, "The base-2 logarithm of the universe to draw numbers from (default: 32)");
    cp.add_size_t('s', "key-seed", options.key_seed, "The seed for random key generation (default: timestamp)");
    cp.add_size_t('t', "op-seed", options.op_seed, "The seed for random operation generation (default: timestamp)");
    cp.add_bytes('b', "batch", options.batch, "The batch size for operations");
    cp.add_double('p', "p-base", options.p_base, "The base probability for inserts/deletes in the corresponding phase (default: 0.3)");
    cp.add_double('r', "p-range", options.p_range, "The probability range for inserts/deletes in the corresponding phase (default: 0.5)");
    cp.add_double('q', "p-query", options.p_query, "The probability for queries, if not the phase's primary operation (default: 0.9)");
    cp.add_string('d', "distribution", options.distr, "The distribution of inserted keys in the universe -- 'uniform' or 'normal' (default: uniform)");
    cp.add_size_t("n-mean", options.n_mean, "The mean for a normal distribution will be U/n_mean (default: 2)");
    cp.add_size_t("n-stddev", options.n_mean, "The standard deviation for a normal distribution will be U/n_stddev (default: 8)");
    cp.add_double("hold", options.hold, "The duration of the hold phase, relative to the duration of the insertion phase (default: 0.25)");
    cp.add_string("csv", options.csv_filename, "The name of the CSV file to write plot data to (default: none)");

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

    // TODO: select distribution

    if(options.universe <= 32) {
        generate<uint32_t>();
    } else if(options.universe <= 40) {
        generate<uint40_t>();
    } else if(options.universe <= 64) {
        generate<uint64_t>();
    } else if(options.universe <= 128) {
        generate<uint128_t>();
    } else if(options.universe <= 256) {
        generate<uint256_t>();
    } else {
        std::cerr << "universe size not supported" << std::endl;
        return -5;
    }
}
