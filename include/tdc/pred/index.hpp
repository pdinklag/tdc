#pragma once

#include <cstdint>
#include <cstddef>

#include <tdc/vec/int_vector.hpp>

#include "binary_search_hybrid.hpp"
#include "result.hpp"

namespace tdc {
namespace pred {

/// \brief Predecessor search using universe-based sampling.
///
/// Using this approach, we define a parameter <tt>k</tt> so that for a predecessor query, we can look up an interval of size at most <tt>2^k</tt> and
/// proceed with a \ref BinarySearchHybrid in that interval.
class Index {
private:
    inline uint64_t hi(uint64_t x) const {
        return x >> m_lo_bits;
    }
    
    size_t m_lo_bits;
    uint64_t m_min, m_max;
    uint64_t m_key_min, m_key_max;

    vec::IntVector m_hi_idx;
    BinarySearchHybrid m_lo_pred;
    
public:
    inline Index() : m_lo_bits(0), m_min(0), m_max(UINT64_MAX), m_key_min(0), m_key_max(UINT64_MAX) {
    }

    /// \brief Constructs the index for the given keys.
    /// \param keys a pointer to the keys, that must be in ascending order
    /// \param num the number of keys
    /// \param lo_bits the number of low key bits, defining the maximum size of a search interval; lower means faster queries, but more memory usage
    Index(const uint64_t* keys, const size_t num, const size_t lo_bits);

    Index(const Index& other) = default;
    Index(Index&& other) = default;
    Index& operator=(const Index& other) = default;
    Index& operator=(Index&& other) = default;

    /// \brief Finds the rank of the predecessor of the specified key.
    /// \param keys the keys that the compressed trie was constructed for
    /// \param num the number of keys
    /// \param x the key in question
    Result predecessor(const uint64_t* keys, const size_t num, const uint64_t x) const;
};

}} // namespace tdc::pred

