#pragma once

#include <string>

namespace tdc {
namespace framework {

class Algorithm {
public:
    virtual std::string id() const = 0;
};

template<typename T>
concept Algorithm = std::is_base_of<Algorithm, T>::value;

}} // namespace tdc::framework
