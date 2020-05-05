#pragma once

#include <cstdint>
#include <cstddef>

#include "result.hpp"

namespace tdc {
namespace pred {

/// \brief Simple binary predecessor search.
class BinarySearch {
private:
    uint64_t m_min, m_max;

public:
    inline BinarySearch() : m_min(0), m_max(UINT64_MAX) {
    }

    /// \brief Initializes a binary search for the given keys.
    /// \param keys a pointer to the keys, that must be in ascending order
    /// \param num the number of keys
    BinarySearch(const uint64_t* keys, const size_t num);

    BinarySearch(const BinarySearch& other) = default;
    BinarySearch(BinarySearch&& other) = default;
    BinarySearch& operator=(const BinarySearch& other) = default;
    BinarySearch& operator=(BinarySearch&& other) = default;

    /// \brief Finds the rank of the predecessor of the specified key.
    /// \param keys the keys that the compressed trie was constructed for
    /// \param num the number of keys
    /// \param x the key in question
    Result predecessor(const uint64_t* keys, const size_t num, const uint64_t x) const;
};

}} // namespace tdc::pred
