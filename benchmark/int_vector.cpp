#include <iostream>
#include <vector>

#include <tdc/random/vector.hpp>
#include <tdc/stat/phase.hpp>
#include <tdc/vec/int_vector.hpp>
#include <tdc/vec/fixed_width_int_vector.hpp>

#include <tlx/cmdline_parser.hpp>

using namespace tdc;

struct {
    size_t num = 1'000'000ULL;
    std::vector<uint64_t> data;
    
    size_t num_queries = 10'000'000ULL;
    std::vector<size_t> queries;

    uint64_t seed = random::DEFAULT_SEED;
} options;

stat::Phase benchmark_phase(std::string&& title) {
    stat::Phase phase(std::move(title));
    phase.log("num", options.num);
    phase.log("queries", options.num_queries);
    phase.log("seed", options.seed);
    return phase;
}

template<typename C>
void bench(C constructor) {
    auto iv = constructor(options.num);
    
    stat::Phase::wrap("set_seq", [&iv](){
        for(size_t i = 0; i < options.num; i++) {
            iv[i] = options.data[i];
        }
    });
    stat::Phase::wrap("get_seq", [&iv](stat::Phase& phase){
        uint64_t chk = 0;
        for(size_t i = 0; i < options.num; i++) {
            chk += iv[i];
        }
		
		auto guard = phase.suppress();
        phase.log("chk", chk);
    });
    stat::Phase::wrap("get_rnd", [&iv](stat::Phase& phase){
        uint64_t chk = 0;
        for(size_t j = 0; j < options.num_queries; j++) {
            const size_t i = options.queries[j];
            chk += iv[i];
        }
		
		auto guard = phase.suppress();
        phase.log("chk", chk);
    });
    stat::Phase::wrap("set_rnd", [&iv](){
        for(size_t j = 0; j < options.num_queries; j++) {
            const size_t i = options.queries[j];
            iv[i] = i + j;
        }
    });
}

template<typename T>
void bench_std_vector(const std::string& name) {
	auto result = benchmark_phase("std");
	
	bench([](const size_t sz){ return std::vector<T>(sz); });
	
	result.suppress([&](){
		std::cout << "RESULT algo=" << name << " " << result.to_keyval() << " " << result.subphases_keyval() << " " << result.subphases_keyval("chk") << std::endl;
	});
}

template<size_t w>
void bench_fwint_vector() {
	auto result = benchmark_phase("FixedWidthIntVector");
	
	bench([](const size_t sz){ return vec::FixedWidthIntVector<w>(sz); });
	
	result.suppress([&](){
		std::cout << "RESULT algo=FixedWidthIntVector<" << w << "> " << result.to_keyval() << " " << result.subphases_keyval() << " " << result.subphases_keyval("chk") << std::endl;
	});
}

void bench_int_vector(const size_t w) {
	auto result = benchmark_phase("IntVector");
	
	bench([w](const size_t sz){ return vec::IntVector(sz, w); });
	
	result.suppress([&](){
		std::cout << "RESULT algo=IntVector(" << w << ") " << result.to_keyval() << " " << result.subphases_keyval() << " " << result.subphases_keyval("chk") << std::endl;
	});
}

int main(int argc, char** argv) {
    tlx::CmdlineParser cp;
    cp.add_bytes('n', "num", options.num, "The size of the bit vetor (default: 1M).");
    cp.add_bytes('q', "queries", options.num_queries, "The size of the bit vetor (default: 10M).");
    cp.add_bytes('s', "seed", options.seed, "The random seed.");
    if(!cp.process(argc, argv)) {
        return -1;
    }

    // generate numbers
    options.data = random::vector<uint64_t>(options.num, UINT64_MAX, options.seed);

    // generate queries
    options.queries = random::vector<size_t>(options.num_queries, options.num - 1, options.seed);
    
	// std::vector
	bench_std_vector<uint8_t>("std<8>");
	bench_std_vector<uint16_t>("std<16>");
	bench_std_vector<uint32_t>("std<32>");
	bench_std_vector<uint64_t>("std<64>");

	// tdc::vec::FixedWidthIntVector
	bench_fwint_vector<2>();
	bench_fwint_vector<3>();
	bench_fwint_vector<4>();
	bench_fwint_vector<5>();
	bench_fwint_vector<6>();
	bench_fwint_vector<7>();
	bench_fwint_vector<8>();
	bench_fwint_vector<9>();
	bench_fwint_vector<10>();
	bench_fwint_vector<11>();
	bench_fwint_vector<12>();
	bench_fwint_vector<13>();
	bench_fwint_vector<14>();
	bench_fwint_vector<15>();
	bench_fwint_vector<16>();
	bench_fwint_vector<24>();
	bench_fwint_vector<32>();
	bench_fwint_vector<40>();
	bench_fwint_vector<48>();
	bench_fwint_vector<56>();
	bench_fwint_vector<63>();
    
    // tdc::vec::IntVector
	bench_int_vector(2);
	bench_int_vector(3);
	bench_int_vector(4);
	bench_int_vector(5);
	bench_int_vector(6);
	bench_int_vector(7);
	bench_int_vector(8);
	bench_int_vector(9);
	bench_int_vector(10);
	bench_int_vector(11);
	bench_int_vector(12);
	bench_int_vector(13);
	bench_int_vector(14);
	bench_int_vector(15);
	bench_int_vector(16);
	bench_int_vector(24);
	bench_int_vector(32);
	bench_int_vector(40);
	bench_int_vector(48);
	bench_int_vector(56);
	bench_int_vector(63);
    
    return 0;
}
