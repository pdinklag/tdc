#pragma once

#include <memory>
#include <string>
#include <vector>
#include <utility>

#include <nlohmann/json.hpp>

namespace tdc::framework {

class AlgorithmInfo;

/**
 * @brief Constraint for types that provide an @refitem AlgorithmInfo.
 * 
 * In order to satisfy this concept, the type must define a static method named @c info that returns an @ref AlgorithmInfo.
 * 
 * @tparam T the type in question
 */
template<typename T>
concept ProvidesAlgorithmInfo = 
    requires(T) {
        { T::info() } -> std::convertible_to<AlgorithmInfo>;
    };

/**
 * @brief Provides registry information on an @ref Algorithm.
 * 
 */
class AlgorithmInfo {
private:
    struct SubAlgorithm {
        std::string                    param_name;
        std::unique_ptr<AlgorithmInfo> info;
    };

    std::string name_;
    std::vector<SubAlgorithm> sub_algorithms_;

public:
    AlgorithmInfo() : name_("") {
    }

    /**
     * @brief Constructs an algorithm information object with the given algorithm name.
     * 
     * @param name the algorithm name
     */
    AlgorithmInfo(std::string&& name) : name_(std::move(name)) {
    }

    AlgorithmInfo(const AlgorithmInfo&) = delete;
    AlgorithmInfo(AlgorithmInfo&& other) = default;

    AlgorithmInfo& operator=(const AlgorithmInfo&) = delete;
    AlgorithmInfo& operator=(AlgorithmInfo&&) = default;

    /**
     * @brief Declares a sub algorithm
     * 
     * @tparam A the sub algorithm type
     * @param param_name the name of the corresponding configuation parameter
     */
    template<typename A>
    requires ProvidesAlgorithmInfo<A>
    void add_sub_algorithm(std::string&& param_name) {
        sub_algorithms_.emplace_back(std::move(param_name), std::make_unique<AlgorithmInfo>(A::info()));
    }

    /**
     * @brief Builds a signature for the represented algorithm and its sub algorithms.
     * 
     * The signature contains only of the algorithm names and no further configuration parmeters.
     * It is used by an @ref Application for matching the given configuration against the available algorithms.
     * 
     * @return the represented algorithm's signature
     */
    nlohmann::json signature() const;

    /**
     * @brief Gets the name of the algorithm.
     * 
     * @return the name of the algorithm.
     */
    inline const std::string& name() const {
        return name_;
    }
};

}
