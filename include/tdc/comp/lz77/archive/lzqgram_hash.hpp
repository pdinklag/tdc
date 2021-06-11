#pragma once

#include <bit>
#include <cstdint>
#include <concepts>
#include <iostream>
#include <limits>
#include <stdexcept>

#include <tdc/math/prime.hpp>
#include <tdc/util/index.hpp>

#include "stats.hpp"

namespace tdc {
namespace comp {
namespace lz77 {
namespace qgram {

template<size_t m_filter_size, std::unsigned_integral qgram_t>
class HashProcessor {
private:
    static constexpr uint64_t m_filter_prime = math::prime_predecessor(m_filter_size);

    struct FilterEntry {
        qgram_t last;
        index_t seen_at;
        
        FilterEntry() : last(0), seen_at(0) {
        }
    } __attribute__((__packed__));

    std::vector<FilterEntry> m_filter;

    uint64_t hash(const qgram_t& key) const {
        return (uint64_t)((key % m_filter_prime) % m_filter_size);
    }

public:
    HashProcessor() : m_filter(m_filter_size, FilterEntry()) {
    }

    template<typename lzqgram_t>
    requires (lzqgram_t::qgram_endian == std::endian::little)
    void process(lzqgram_t& c, std::ostream& out) {
        auto prefix = c.qgram();
        size_t len = lzqgram_t::q;
        
        const size_t jmax = lzqgram_t::q - (c.threshold()-1);
        for(size_t j = 0; j < jmax; j++) {
            // test if prefix has been seen before
            const auto h = hash(prefix);
            if(m_filter[h].last == prefix) {
                const auto src = m_filter[h].seen_at;
                m_filter[h].seen_at = c.pos();
                
                c.output_ref(out, src, len);
            } else {
                if constexpr(lzqgram_t::track_stats) {
                    if(m_filter[h].last) {
                        ++c.stats().num_collisions;
                    } else {
                        ++c.stats().trie_size;
                    }
                }
                m_filter[h].last = prefix;
                m_filter[h].seen_at = c.pos();
            }
            
            prefix >>= lzqgram_t::char_bits;
            --len;
        }
    }
};

}}}} // namespace tdc::comp::lz77::qgram
