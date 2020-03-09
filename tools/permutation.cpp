#include <cassert>
#include <iostream>

#include <tdc/stat/time.hpp>
#include <tdc/random/permutation.hpp>

#include <tlx/cmdline_parser.hpp>

using namespace tdc;

int main(int argc, char** argv) {
    tlx::CmdlineParser cp;
    cp.set_description("Draws numbers from a random permutation and prints them to the standard output.");

    uint32_t num = 10ULL;
    cp.add_bytes('n', "num", num, "The number of numbers to generate (default: 10).");

    size_t u = 0xFFFFFFFFULL;
    cp.add_bytes('u', "universe", u, "The universe to draw numbers from (default: 32 bits).");

    size_t seed = stat::time_nanos();
    cp.add_size_t('s', "seed", seed, "The seed for random generation (default: timestamp).");

#ifndef NDEBUG
    bool check = false;
    cp.add_flag('c', "check", check, "Check that a permutation is generated (debug).");
#endif

    if(!cp.process(argc, argv)) {
        return -1;
    }

    if(u < num) {
        std::cerr << "the universe must be at least as large as the number of generated numbers" << std::endl;
        return -2;
    }

    // generate numbers
    auto perm = random::Permutation(u, seed);

#ifndef NDEBUG
    if(check) {
        std::vector<bool> b(u);
        for(uint64_t i = 0; i < u; i++) {
            const uint64_t j = perm(i);
            assert(!b[j]);
            b[j] = 1;
        }
    }
#endif

    for(uint64_t i = 0; i < num; i++) {
        std::cout << perm(i) << std::endl;
    }

    return 0;
}
