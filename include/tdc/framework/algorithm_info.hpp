#pragma once

#include <memory>
#include <string>
#include <vector>
#include <utility>

#include <nlohmann/json.hpp>

namespace tdc::framework {

class AlgorithmInfo;

template<typename T>
concept ProvidesAlgorithmInfo = 
    requires(T) {
        { T::info() } -> std::convertible_to<AlgorithmInfo>;
    };

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

    AlgorithmInfo(std::string&& name) : name_(std::move(name)) {
    }

    AlgorithmInfo(const AlgorithmInfo&) = delete;
    AlgorithmInfo(AlgorithmInfo&& other) = default;

    AlgorithmInfo& operator=(const AlgorithmInfo&) = delete;
    AlgorithmInfo& operator=(AlgorithmInfo&&) = default;

    template<typename A>
    requires ProvidesAlgorithmInfo<A>
    void add_sub_algorithm(std::string&& param_name) {
        sub_algorithms_.emplace_back(std::move(param_name), std::make_unique<AlgorithmInfo>(A::info()));
    }

    nlohmann::json signature() const;

    inline const std::string& name() const {
        return name_;
    }
};

}
