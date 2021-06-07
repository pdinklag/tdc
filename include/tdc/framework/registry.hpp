#pragma once

#include <iostream> // DEBUG
#include <memory>

#include <tdc/framework/algorithm.hpp>
#include <tdc/util/type_list.hpp>

namespace tdc {
namespace framework {

class Registry {
public:
    template<Algorithm A>
    void register_algorithm() {
        auto a = std::make_unique<A>();
        std::cout << a->id() << std::endl;
    }

    inline void register_algorithms(tl::empty) {
        // nothing to do
    }

    template<Algorithm A, Algorithm... As>
    void register_algorithms(tl::list<A, As...>) {
        register_algorithm<A>();
        register_algorithms(tl::list<As...>());
    }
};

}} // namespace tdc::framework
