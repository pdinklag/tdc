#include <iostream>
#include <vector>

#include <tdc/random/permutation.hpp>
#include <tdc/random/vector.hpp>
#include <tdc/stat/phase.hpp>

#include <tdc/pred/dynamic/dynamic_fusion_node_8.hpp>

#include <tlx/cmdline_parser.hpp>

int main(int argc, char** argv) {
    using namespace tdc::pred::dynamic;

    DynamicFusionNode8 node;
    node.print(); std::cout << std::endl;
    node.insert(10);
    node.print(); std::cout << std::endl;
    node.insert(25);
    node.print(); std::cout << std::endl;
    node.insert(17);
    node.print(); std::cout << std::endl;
    node.remove(10);
    node.print(); std::cout << std::endl;
    node.insert(23);
    node.print(); std::cout << std::endl;
    node.insert(12);
    node.print(); std::cout << std::endl;
    node.insert(9);
    node.print(); std::cout << std::endl;
    node.insert(20);
    node.print(); std::cout << std::endl;
    node.remove(23);
    node.print(); std::cout << std::endl;
    node.insert(1);
    node.print(); std::cout << std::endl;
    return 0;
}
