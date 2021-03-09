#pragma once

#include <bit>
#include <cstdint>
#include <concepts>
#include <iostream>
#include <limits>
#include <stdexcept>

#include <tdc/util/index.hpp>

#include "tries.hpp"
#include "stats.hpp"

namespace tdc {
namespace comp {
namespace lz77 {
namespace qgram {

template<typename trie_t>
class TrieProcessor {
private:
    trie_t m_trie;

public:
    template<typename lzqgram_t>
    void process(lzqgram_t& c, std::ostream& out) {
        using char_t = lzqgram_t::char_t;
        
        // advance in trie as much as possible
        const auto _qgram = c.qgram();
        const char_t* qgram = (const char_t*)&_qgram;
        
        size_t len = 0;
        size_t src = 0;
        
        auto* trie = &m_trie;
        for(size_t j = 0; j < lzqgram_t::q; j++) {
            const size_t i = lzqgram_t::q - 1 - j; // little endian!
            trie = trie->get_or_create_child(qgram[i]);
            if(trie->count()) {
                ++len; // nb: this assumes that if a q-gram does not exist in the trie, neither does any (q+1)-gram extending it
                src = trie->last_seen_at();
            } else {
                // node was just created
                if constexpr(lzqgram_t::track_stats) ++c.stats().trie_size;
            }
            
            trie->update(trie->count() + 1, c.pos());
        }
        
        c.output_ref(out, src, len);
    }
};

}}}} // namespace tdc::comp::lz77::qgram