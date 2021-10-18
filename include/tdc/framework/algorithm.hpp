#pragma once

#include <tdc/framework/algorithm_info.hpp>

namespace tdc {
namespace framework {

class Algorithm {
};

template<typename T>
concept Algorithm = 
    std::derived_from<T, Algorithm> &&
    std::default_initializable<T> &&
    ProvidesAlgorithmInfo<T>;

}} // namespace tdc::framework
