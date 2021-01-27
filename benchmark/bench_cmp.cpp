#include <cassert>
#include <cstdint>
#include <iostream>
#include <limits>
#include <random>
#include <string>
#include <utility>

#include <tdc/intrisics/pcmp.hpp>
#include <tdc/intrisics/tzcnt.hpp>
#include <tdc/stat/phase.hpp>

#include <tlx/cmdline_parser.hpp>

template<typename elem_t, typename matrix_t>
size_t rank_linear(const elem_t e, const matrix_t& m) {
    constexpr size_t num = sizeof(matrix_t) / sizeof(elem_t);
    const elem_t* array = (const elem_t*)&m;
    size_t i = 0;
    while(i < num && e >= array[i]) {
        ++i;
    }
    return i;
}

struct {
    size_t num = 1'000'000;
    size_t seed = 147;
    uint64_t matrix = 0xF3'E2'CC'91'7F'44'25'01ULL;
} options;

template<typename T>
struct repeat;

template<>
struct repeat<uint8_t> {
    inline uint64_t operator()(const uint8_t x) {
        return uint64_t(x) * 0x01'01'01'01'01'01'01'01ULL;
    }
};

template<typename elem_t, typename matrix_t>
size_t rank_pcmp(const elem_t e, const matrix_t& m) {
    const matrix_t e_repeat = repeat<elem_t>{}(e);
    const auto cmp = tdc::intrisics::pcmpgtu<matrix_t, elem_t>(m, e_repeat);
    const size_t rank = tdc::intrisics::tzcnt0(cmp) / std::numeric_limits<elem_t>::digits;
#ifndef NDEBUG
    const size_t correct = rank_linear(e, m);
    assert(rank == correct);
#endif
    return rank;
}

template<typename F>
void bench(const std::string& name, F rank_func) {
    std::default_random_engine gen(options.seed);
    std::uniform_int_distribution<uint8_t> dist(0, UINT8_MAX);
    
    tdc::stat::Phase phase("rank");
    size_t chk = 0;
    for(size_t i = 0; i < options.num; i++) {
        chk += rank_func(dist(gen), options.matrix);
    }
    
    auto guard = phase.suppress();
    phase.log("chk", chk);
    std::cout << "RESULT algo=" << name << " " << phase.to_keyval() << std::endl;
}

int main(int argc, char** argv) {
    tlx::CmdlineParser cp;
    cp.add_bytes('n', "num", options.num, "The length of the sequence (default: 1M).");
    cp.add_bytes('s', "seed", options.seed, "The random seed.");
    if(!cp.process(argc, argv)) {
        return -1;
    }

    bench("linear", [](uint8_t x, uint64_t m){ return rank_linear(x, m); });
    bench("pcmp", [](uint8_t x, uint64_t m){ return rank_pcmp(x, m); });
}
