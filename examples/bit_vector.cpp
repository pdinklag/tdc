#include <iostream>

#include <tdc/vec/bit_vector.hpp>

int main(int argc, char** argv) {
    tdc::vec::BitVector bv(1'000);
    bv[1] = 1;
    std::cout << bv[0] << ", " << bv[1] << std::endl;

    tdc::vec::BitVector cp = bv;
    std::cout << cp[0] << ", " << cp[1] << std::endl;

    tdc::vec::BitVector mv = std::move(bv);
    std::cout << mv[0] << ", " << mv[1] << std::endl;
    
    return 0;
}
