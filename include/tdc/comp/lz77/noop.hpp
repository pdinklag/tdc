#pragma once

#include <tdc/util/literals.hpp>

#include "factor_buffer.hpp"

namespace tdc {
namespace comp {
namespace lz77 {

class Noop {
public:
    Noop() {
    }

    template<typename FactorOutput>
    void compress(std::istream& in, FactorOutput& out) {
        // simply echo input
        constexpr size_t bufsize = 1_Mi;
        char* buffer = new char[bufsize];
        bool b;
        do {
            b = (bool)in.read(buffer, bufsize);
            const size_t num = in.gcount();
            for(size_t i = 0; i < num; i++) {
                
                out.emplace_back((char_t)buffer[i]);
            }
        } while(b);
        delete[] buffer;
    }
    
    template<typename StatLogger>
    void log_stats(StatLogger& logger) {
    }
};

}}} // namespace tdc::comp::lz77
