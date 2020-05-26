#include <iostream>
#include <vector>

#include <tdc/random/permutation.hpp>
#include <tdc/random/vector.hpp>
#include <tdc/stat/phase.hpp>

#include <tdc/pred/dynamic/dynamic_fusion_node_8.hpp>

#include <tlx/cmdline_parser.hpp>

using namespace tdc::pred::dynamic;

const uint64_t TEST = 10;
DynamicFusionNode8 node;

void pred(const uint64_t x) {
    auto r = node.predecessor(x);
    std::cout << "\tpredecessor of " << x;
    if(r.exists) {
        std::cout << " is " << node[r.pos];
    } else {
        std::cout << " does not exist";
    }
    std::cout << std::endl;
}

int main(int argc, char** argv) {
    node.print(); pred(TEST); std::cout << std::endl;
    node.insert(10);
    node.print(); pred(TEST); std::cout << std::endl;
    node.insert(25);
    node.print(); pred(TEST); std::cout << std::endl;
    node.insert(17);
    node.print(); pred(TEST); std::cout << std::endl;
    node.remove(10);
    node.print(); pred(TEST); std::cout << std::endl;
    node.insert(23);
    node.print(); pred(TEST); std::cout << std::endl;
    node.insert(12);
    node.print(); pred(TEST); std::cout << std::endl;
    node.insert(9);
    node.print(); pred(TEST); std::cout << std::endl;
    node.insert(20);
    node.print(); pred(TEST); std::cout << std::endl;
    node.remove(23);
    node.print(); pred(TEST); std::cout << std::endl;
    node.insert(1);
    node.print(); pred(TEST); std::cout << std::endl;
    return 0;
}
