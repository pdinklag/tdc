#pragma once

#include <tdc/util/literals.hpp>

#include "stats.hpp"

namespace tdc {
namespace comp {
namespace lz77 {

template<bool m_track_stats = false>
class Noop {
private:
    Stats m_stats;

public:
    Noop() {
    }
    
    void compress(std::istream& in, std::ostream& out) {
        // simply echo input
        constexpr size_t bufsize = 1_Mi;
        char* buffer = new char[bufsize];
        bool b;
        do {
            b = (bool)in.read(buffer, bufsize);
            const size_t num = in.gcount();
            for(size_t i = 0; i < num; i++) {
                out << buffer[i];
            }
            if constexpr(m_track_stats) m_stats.input_size += num;
        } while(b);
        delete[] buffer;
     
        if constexpr(m_track_stats) m_stats.num_literals = m_stats.input_size;
    }
    
    const Stats& stats() const { return m_stats; }
};

}}} // namespace tdc::comp::lz77
