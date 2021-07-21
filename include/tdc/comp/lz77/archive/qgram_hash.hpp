#pragma once

#include <concepts>
#include <cstddef>
#include <limits>

#include <tdc/intrisics/lzcnt.hpp>
#include <tdc/uint/uint128.hpp>

namespace tdc {
namespace comp {
namespace lz77 {
namespace qgram {

template<std::unsigned_integral qgram_t>
class QGramHash {
private:
    using char_t = unsigned char;
    static constexpr size_t q = sizeof(qgram_t);
    static constexpr size_t q_bits = std::numeric_limits<qgram_t>::digits;

    //static constexpr uint128_t PRIME = 9223372036854775837ULL; // 2^63 + 29
    static constexpr uint64_t PRIME = (1ULL << 62) - 273ULL;

public:
    uint64_t operator()(const qgram_t& key) const {
        const size_t lzcnt = intrisics::lzcnt(key); // number of leading zeroes
        const size_t nzcnt = q_bits - lzcnt; // number of trailing bits
        const size_t extend = lzcnt / nzcnt;

        qgram_t xkey = key;
        for(size_t i = 0; i < extend; i++) {
            xkey = (xkey << nzcnt) | key;
        }

        if constexpr(q_bits > 64) {
            uint128_t h = 0;
            for(size_t i = 0; i < q_bits / 64; i++) {
                h = ((h << 64) | (uint64_t)xkey) % uint128_t(PRIME);
                xkey >>= 64;
            }
            return (uint64_t)h;
        } else {
            return xkey % qgram_t(PRIME);
        }
    }
};

}}}} // namespace tdc::comp::lz77::qgram
