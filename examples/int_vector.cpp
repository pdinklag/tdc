#include <iostream>

#include <tdc/vec/int_vector.hpp>

int main(int argc, char** argv) {
    tdc::vec::IntVector iv(1'000, 6);
    iv[1] = 42;
    std::cout << iv[0] << ", " << iv[1] << std::endl;

    tdc::vec::IntVector cp = iv;
    std::cout << cp[0] << ", " << cp[1] << std::endl;

    tdc::vec::IntVector mv = std::move(iv);
    std::cout << mv[0] << ", " << mv[1] << std::endl;
    
    return 0;
}
