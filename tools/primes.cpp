#include <cassert>
#include <iostream>

#include <tdc/math/prime.hpp>

#include <tlx/cmdline_parser.hpp>

using namespace tdc;

int main(int argc, char** argv) {
    tlx::CmdlineParser cp;
    cp.set_description("Lists prime numbers.");

    uint32_t num = 10ULL;
    cp.add_bytes('n', "num", num, "The number of primes to generate (default: 10).");

    uint64_t start = 2;
    cp.add_bytes('s', "start", start, "All primes must be greater than or equal to this value (default: 2)");
    
    uint64_t step = 1;
    cp.add_bytes('d', "step", step, "Consecutive primes should at least be this far apart (default: 1)");

    if(!cp.process(argc, argv)) {
        return -1;
    }

    if(num > 0) {
        uint64_t p = tdc::math::prime_successor(start);
        while(num--) {
            std::cout << p << std::endl;
            if(num) p = tdc::math::prime_successor(p + step);
        }
    }

    return 0;
}
