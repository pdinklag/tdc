#include <cstdint>
#include <iostream>

#include <tdc/sketch/count_min.hpp>

int main(int argc, char** argv) {
    tdc::sketch::CountMin<char> sketch(16, 8);
    
    sketch.process('A');
    sketch.process('A');
    sketch.process('B');
    sketch.process('A');
    sketch.process('B');
    sketch.process('A');
    sketch.process('C');
    sketch.process('A');
    sketch.process('D');
    sketch.process('E');
    
    std::cout << "#A: " << sketch.count('A') << std::endl;
    std::cout << "#B: " << sketch.count('B') << std::endl;
    std::cout << "#C: " << sketch.count('C') << std::endl;
    std::cout << "#D: " << sketch.count('D') << std::endl;
    std::cout << "#E: " << sketch.count('E') << std::endl;
}
