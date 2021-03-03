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
#include <tdc/util/index.hpp>

namespace tdc {
namespace sketch {

/// \brief An implementation of the count-min sketch by Cormode & Muthukrishnan, 2004.
template<typename key_t, typename _count_t = index_t>
requires std::unsigned_integral<_count_t> && std::totally_ordered<_count_t>
class CountMin {
public:
    using count_t = _count_t;

private:
    size_t m_num_rows, m_num_cols;
    std::vector<hash::Multiplicative> m_hash;
    std::vector<std::vector<count_t>> m_data;

    size_t hash(size_t i, const key_t& key) const {
        return m_hash[i](key) % m_num_cols;
    }

public:
    CountMin() : m_num_rows(0), m_num_cols(0) {
    }

    /// \brief Constructor.
    /// \param num_cols the number of columns
    /// \param num_rows the number of rows
    /// \param seed the random seed for hash function selection
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
    
    CountMin(const CountMin& other) = default;
    CountMin(CountMin&& other) = default;
    
    CountMin& operator=(const CountMin& other) = default;
    CountMin& operator=(CountMin&& other) = default;

    /// \brief Processes a key.
    /// \param key the key in question
    /// \param c the number of times to count the key
    void process(const key_t& key, count_t c = count_t(1)) {
        for(size_t i = 0; i < m_num_rows; i++) {
            const auto h = hash(i, key);
            m_data[i][h] += c;
        }
    }

    /// \brief Processes a key and reports its new count estimate.
    /// \param key the key in question
    /// \param c the number of times to count the key
    count_t process_and_count(const key_t& key, count_t c = count_t(1)) {
        count_t min = std::numeric_limits<count_t>::max();
        for(size_t i = 0; i < m_num_rows; i++) {
            const auto h = hash(i, key);
            m_data[i][h] += c;
            min = std::min(min, m_data[i][h]);
        }
        return min;
    }

    /// \brief Reports a count estimate for the given key.
    /// \param key the key in question
    count_t count(const key_t& key) const {
        count_t min = std::numeric_limits<count_t>::max();
        for(size_t i = 0; i < m_num_rows; i++) {
            const auto h = hash(i, key);
            min = std::min(min, m_data[i][h]);
        }
        return min;
    }
};

}} // namespace tdc::sketch