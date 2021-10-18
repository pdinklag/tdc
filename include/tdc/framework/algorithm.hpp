#pragma once

#include <tdc/framework/algorithm_info.hpp>
#include <tdc/framework/config.hpp>

namespace tdc::framework {

class Algorithm {
public:
    virtual void configure(const Config& config) {
    }
};

template<typename T>
concept Algorithm = 
    std::derived_from<T, Algorithm> &&
    std::default_initializable<T> &&
    ProvidesAlgorithmInfo<T>;

}
