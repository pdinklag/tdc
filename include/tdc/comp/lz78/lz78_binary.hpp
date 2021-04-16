#pragma once

#include <iostream>

#include "stats.hpp"

#include <tdc/comp/lz78/binary_trie.hpp>
#include <tdc/util/literals.hpp>

namespace tdc {
namespace comp {
namespace lz78 {

template<bool m_track_stats = false>
class LZ78Binary {
private:
    Stats m_stats;
    
    BinaryTrie<char> m_trie;
    index_t m_current;

    void process(char c, std::ostream& out) {
        auto child = m_trie.get_child(m_current, c);
        if(child) {
            m_current = child;
        } else {
            out << "(" << m_current << "," << c << ")";
            m_trie.insert_child(m_current, c);
            m_current = m_trie.root();
        }
    }

public:
    LZ78Binary() {
        m_current = m_trie.root();
    }
    
    void compress(std::istream& in, std::ostream& out) {
        constexpr size_t bufsize = 1_Mi;
        char* buffer = new char[bufsize];
        bool b;
        do {
            b = (bool)in.read(buffer, bufsize);
            const size_t num = in.gcount();
            for(size_t i = 0; i < num; i++) {
                process(buffer[i], out);
            }
            if constexpr(m_track_stats) m_stats.input_size += num;
        } while(b);
        
        // output final factor
        if(m_current) {
            out << "(" << m_current << ",<EOF>)";
        }
        
        delete[] buffer;
        if constexpr(m_track_stats) m_stats.trie_size = m_trie.size();
    }
    
    const Stats& stats() const { return m_stats; }
};

}}}
