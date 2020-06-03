#include <iostream>
#include <vector>

#include <tdc/random/permutation.hpp>
#include <tdc/random/vector.hpp>
#include <tdc/stat/phase.hpp>

#include <tdc/pred/dynamic/dynamic_octrie.hpp>

#include <tlx/cmdline_parser.hpp>

using namespace tdc;
using namespace tdc::pred::dynamic;

int main(int argc, char** argv) {
    // generate and insert numbers
    stat::Phase result("Result");
    {
        
        DynamicOctrie tree;
        auto perm = random::Permutation(1'000, 147);
        for(size_t i = 0; i < 100; i++) {
            const uint64_t x = perm(i);
            tree.insert(x);
        }
        
        result.suppress([&](){
            tree.print();
            std::cout << "predecessor of 3 is " << tree.predecessor(3).pos << std::endl;
            std::cout << "predecessor of 42 is " << tree.predecessor(42).pos << std::endl;
            std::cout << "predecessor of 5 is " << tree.predecessor(5).pos << std::endl;
            std::cout << "predecessor of 35 is " << tree.predecessor(35).pos << std::endl;
            std::cout << "predecessor of 135 is " << tree.predecessor(135).pos << std::endl;
            std::cout << "predecessor of 100 is " << tree.predecessor(100).pos << std::endl;
            std::cout << "predecessor of 9999 is " << tree.predecessor(9999).pos << std::endl;
            result.log("size", tree.size());
        });
    }
    
    result.suppress([&](){
        std::cout << "RESULT " << result.to_keyval() << std::endl;
    });
    
    return 0;
}
