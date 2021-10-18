#pragma once

#include <tdc/framework/algorithm_info.hpp>
#include <tdc/framework/config.hpp>

namespace tdc::framework {

/**
 * @brief Base class for algorithms.
 * 
 */
class Algorithm {
public:
    /**
     * @brief Configures the algorithm.
     * 
     * @param config the configuration
     */
    virtual void configure(const Config& config) {
    }
};

/**
 * @brief Constraint for @ref Algorithm "Algorithms".
 * 
 * @tparam T the type in question
 */
template<typename T>
concept Algorithm = 
    std::derived_from<T, Algorithm> &&
    std::default_initializable<T> &&
    ProvidesAlgorithmInfo<T>;

}
