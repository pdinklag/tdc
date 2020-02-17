#include <iostream>

#include <tdc/stat/phase.hpp>
#include <tdc/rapl/rapl_phase_extension.hpp>

int main(int argc, char** argv) {
    tdc::stat::Phase::register_extension<tdc::rapl::RAPLPhaseExtension>();
    
    tdc::stat::Phase root("Root");
    {
        constexpr uint32_t n = 1'000'000;
        root.log("n", n);
        
        uint32_t* array = new uint32_t[n];
        for(size_t i = 0; i < n; i++) {
            array[i] = n - i;
        }
        delete[] array;
    }

    std::cout << root.to_json().dump(4) << std::endl;
    std::cout << "RESULT " << root.to_keyval() << std::endl;
    return 0;
}
