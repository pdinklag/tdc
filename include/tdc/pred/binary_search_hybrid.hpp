#pragma once

#include <cstdint>
#include <cstddef>

#include "result.hpp"

namespace tdc {
namespace pred {

/// \brief Binary predecessor search that switches to linear search in small intervals.
class BinarySearchHybrid {
private:
    uint64_t m_min, m_max;
    size_t m_cache_num;

public:
    inline BinarySearchHybrid() : m_min(0), m_max(UINT64_MAX), m_cache_num(0) {
    }

    /// \brief Initializes a hybrid search for the given keys.
    /// \param keys a pointer to the keys, that must be in ascending order
    /// \param num the number of keys
    /// \param cache_num the size of the search interval below which linear search will be used instead
    BinarySearchHybrid(const uint64_t* keys, const size_t num, const size_t cache_num = 512ULL / sizeof(uint64_t));

    BinarySearchHybrid(const BinarySearchHybrid& other) = default;
    BinarySearchHybrid(BinarySearchHybrid&& other) = default;
    BinarySearchHybrid& operator=(const BinarySearchHybrid& other) = default;
    BinarySearchHybrid& operator=(BinarySearchHybrid&& other) = default;

    /// \brief Finds the rank of the predecessor of the specified key in the given interval.
    /// \param keys the keys that the compressed trie was constructed for
    /// \param p the left search interval border
    /// \param q the right search interval border
    /// \param x the key in question
    PosResult predecessor_seeded(const uint64_t* keys, size_t p, size_t q, const uint64_t x) const;
    
    /// \brief Finds the rank of the predecessor of the specified key.
    /// \param keys the keys that the compressed trie was constructed for
    /// \param num the number of keys
    /// \param x the key in question
    PosResult predecessor(const uint64_t* keys, const size_t num, const uint64_t x) const;
};

}} // namespace tdc::pred
