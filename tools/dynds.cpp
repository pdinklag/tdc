#include <iostream>
#include <random>
#include <set>

#include <tdc/pred/dynamic/btree.hpp>
#include <tdc/pred/dynamic/btree/sorted_array_node.hpp>
#include <tdc/stat/time.hpp>
#include <tdc/util/benchmark/integer_operation.hpp>

#include <tlx/cmdline_parser.hpp>

using namespace tdc;

int main(int argc, char** argv) {
    tlx::CmdlineParser cp;
    cp.set_description("Generates a sequence of inserts, queries and deletes to simulate operation of a dynamic data structure.");

    size_t max_num = 100ULL;
    cp.add_bytes('n', "num", max_num, "The maximum number of items in the data structure (default: 100).");
    
    size_t max_ops = SIZE_MAX;
    cp.add_bytes('m', "max-ops", max_ops, "The maximum number of operations to generate (default: infinite).");
    
    uint64_t u = 32;
    cp.add_bytes('u', "universe", u, "The base-2 logarithm of the universe to draw numbers from (default: 32).");

    size_t key_seed = stat::time_nanos();
    cp.add_size_t('s', "key-seed", key_seed, "The seed for random key generation (default: timestamp).");

    size_t op_seed = stat::time_nanos();
    cp.add_size_t('t', "op-seed", op_seed, "The seed for random operation generation (default: timestamp).");
    
    float p_base = 0.3f;
    cp.add_float('p', "p-base", p_base, "The base probability for inserts/deletes in the corresponding phase (default: 0.3)");
    
    float p_range = 0.5f;
    cp.add_float('r', "p-range", p_range, "The probability range for inserts/deletes in the corresponding phase (default: 0.5)");
    
    float p_query = 0.9f;
    cp.add_float('q', "p-query", p_query, "The probability for queries, if not the phase's primary operation (default: 0.9)");
    
    float hold = 0.25f;
    cp.add_float("hold", hold, "The duration of the hold phase, relative to the duration of the insertion phase (default: 0.25)");

    bool binary = false;
    cp.add_flag("binary", binary, "Use binary output -- each operation is then output as the 8-bit character followed by an unsigned 64-bit integer.");
    
    bool print_opnum = false;
    cp.add_flag("print-opnum", print_opnum, "Print the number of each operation.");
    
    bool print_num = false;
    cp.add_flag("print-num", print_num, "Print the number of items after each operation.");

    if(!cp.process(argc, argv)) {
        return -1;
    }
    
    u = (u < 64) ? ((1ULL << u)-1) : UINT64_MAX;
    if(u < max_num) {
        std::cerr << "the universe must be at least as large as the maximum number of items" << std::endl;
        return -2;
    }
    
    if(p_base + p_range >= 1.0f) {
        std::cerr << "p_base + p_range must be less than one" << std::endl;
        return -3;
    }
    
    if(p_query >= 1.0f) {
        std::cerr << "p_query must be less than one" << std::endl;
        return -4;
    }

    // random generator
    std::mt19937 gen_op(op_seed);
    std::uniform_real_distribution<float>   random_op(0.0f, 1.0f);
    
    std::mt19937 gen_val(key_seed);
    std::uniform_int_distribution<uint64_t> random_from_universe(0ULL, u);
    
    // working set
    pred::dynamic::BTree<uint64_t, 65, pred::dynamic::SortedArrayNode<uint64_t, 64>> cur_set;
    uint64_t* cur_arr = new uint64_t[max_num];
    
    uint64_t cur_num = 0;
    uint64_t cur_min = UINT64_MAX;
    uint64_t cur_max = 0;
    
    // operations
    struct op_t {
        char code;
        uint64_t x;
    } __attribute__((packed));
    static_assert(sizeof(op_t) == 9);
    
    size_t count_insert = 0;
    size_t count_delete = 0;
    size_t count_query = 0;
    size_t failed_inserts = 0;
    
    auto generate_insert = [&](){
        ++count_insert;
        
        uint64_t x = random_from_universe(gen_val);
        while(cur_set.contains(x)) {
            x = random_from_universe(gen_val);
            ++failed_inserts;
        }
        
        cur_set.insert(x);
        cur_arr[cur_num] = x;
        
        ++cur_num;
        cur_min = std::min(cur_min, x);
        cur_max = std::max(cur_max, x);
        
        return benchmark::IntegerOperation { benchmark::OPCODE_INSERT, x };
    };
    
    auto generate_query = [&](){
        ++count_query;
        
        std::uniform_int_distribution<uint64_t> random_range(cur_min, cur_max);
        return benchmark::IntegerOperation { benchmark::OPCODE_QUERY, random_range(gen_val) };
    };
    
    auto generate_delete = [&](){
        ++count_delete;
        
        std::uniform_int_distribution<size_t> random_index(0, cur_num - 1);
        const size_t i = random_index(gen_val);
        const uint64_t x = cur_arr[i];
        
        cur_set.remove(x);
        cur_arr[i] = cur_arr[cur_num - 1]; // "delete" = swap with last, order isn't important
        
        --cur_num;
        if(cur_num > 0) {
            if(x == cur_min) cur_min = cur_set.min();
            if(x == cur_max) cur_max = cur_set.max();
        } else {
            cur_min = UINT64_MAX;
            cur_max = 0;
        }

        return benchmark::IntegerOperation { benchmark::OPCODE_DELETE, x };
    };
    
    size_t count_total = 0;
    
    auto output = [&](const benchmark::IntegerOperation& op){
        ++count_total;

        // print
        if(binary) {
            std::cout.write((const char*)&op, sizeof(op));
        } else {
            if(print_opnum) std::cout << count_total << '\t';
            std::cout << op.code << '\t' << op.key;
            if(print_num) std::cout << '\t' << cur_num;
            std::cout << std::endl;
        }
    };
    
    auto insert_probability = [&](){
        return p_base + p_range * float(max_num - cur_num) / float(max_num);
    };
    
    // insertion phase
    while(cur_num < max_num && count_total < max_ops) {
        if(cur_num == 0 || random_op(gen_op) <= insert_probability()) {
            output(generate_insert());
            continue;
        }
        
        if(cur_num > 0 && random_op(gen_op) <= p_query) {
            output(generate_query());
            continue;
        }
        
        else {
            output(generate_delete());
        }
    }
    
    // hold phase
    const size_t max_hold_ops = size_t(hold * float(count_total));
    for(size_t i = 0; i < max_hold_ops && count_total < max_ops; i++) {
        const float p = random_op(gen_op);
        
        if(cur_num < max_num) {
            if(p <= 0.3333f) {
                output(generate_insert());
            } else if(p <= 0.6667f) {
                output(generate_delete());
            } else {
                output(generate_query());
            }
        } else {
            if(p <= 0.5f) {
                output(generate_delete());
            } else {
                output(generate_query());
            }
        }
    }
    
    // deletion phase
    while(cur_num > 0 && count_total < max_ops) {
        if(cur_num == max_num || random_op(gen_op) <= insert_probability()) {
            output(generate_delete());
            continue;
        }
        
        if(cur_num > 0 && random_op(gen_op) <= p_query) {
            output(generate_query());
            continue;
        }
        
        else {
            output(generate_insert());
        }
    }
    
    std::cerr << "generated " << count_total << " operations (key seed: " << key_seed << ", op seed: " << op_seed << ", " << failed_inserts << " duplicates prevented): "
        << count_insert << " inserts, "
        << count_delete << " deletes and "
        << count_query << " queries"
        << std::endl;

    // clean up
    delete[] cur_arr;
}
