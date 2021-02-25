#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <concepts>
#include <limits>
#include <utility>
#include <vector>

#include <tdc/hash/function.hpp>
#include <tdc/math/prime.hpp>
#include <tdc/random/permutation.hpp>

namespace tdc {
namespace sketch {

/// \brief An implementation of the count-min sketch by Cormode & Muthukrishnan, 2004
template<typename key_t, typename count_t = size_t>
requires std::unsigned_integral<count_t> && std::totally_ordered<count_t>
class CountMin {
private:
    size_t m_num_rows, m_num_cols;
    std::vector<hash::Multiplicative> m_hash;
    std::vector<std::vector<count_t>> m_data;

    size_t hash(size_t i, const key_t& key) const {
        return m_hash[i](key) % m_num_cols;
    }

public:
    CountMin(size_t num_cols, size_t num_rows, uint64_t seed = random::DEFAULT_SEED) : m_num_rows(num_rows), m_num_cols(num_cols) {
        // initialize hash functions and data rows
        assert(m_num_rows <= math::NUM_POOL_PRIMES);
        random::Permutation perm(math::NUM_POOL_PRIMES, seed);
        
        m_hash.reserve(m_num_rows);
        m_data.reserve(m_num_rows);
        
        for(size_t i = 0; i < m_num_rows; i++) {
            m_hash.emplace_back(hash::Multiplicative(math::PRIME_POOL[perm(i)]));
            m_data.emplace_back(std::vector<count_t>(m_num_cols, 0));
        }
    }

    void process(const key_t& key, count_t c = count_t(1)) {
        for(size_t i = 0; i < m_num_rows; i++) {
            m_data[i][hash(i, key)] += c;
        }
    }

    count_t count(const key_t& key) const {
        count_t min = std::numeric_limits<count_t>::max();
        for(size_t i = 0; i < m_num_rows; i++) {
            min = std::min(min, m_data[i][hash(i, key)]);
        }
        return min;
    }
};

}} // namespace tdc::sketch
