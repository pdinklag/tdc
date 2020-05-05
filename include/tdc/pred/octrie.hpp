#pragma once

#include "compressed_trie8.hpp"

#include <vector>

namespace tdc {
namespace pred {

/// \brief A cascade of \ref CompressedTrie8 instances in an octree to index a sorted sequence of arbitrary length.
class Octrie {
private:
    struct octree_level_t {
        size_t first_node;
        std::vector<CompressedTrie8> nodes;
    };

    std::vector<octree_level_t> m_octree;
    CompressedTrie8* m_root;
    
    size_t m_octree_size_ub;
    size_t m_height;

public:
    /// \brief Constructs an empty trie
    inline Octrie() : m_root(nullptr), m_octree_size_ub(0), m_height(0) {
    }

    /// \brief Constructs a compressed trie for the given keys.
    /// \param keys a pointer to the keys, that must be in ascending order
    /// \param num the number of keys
    Octrie(const uint64_t* keys, const size_t num);
    
    Octrie(const Octrie& other) = default;
    Octrie(Octrie&& other) = default;
    Octrie& operator=(const Octrie& other) = default;
    Octrie& operator=(Octrie&& other) = default;
    
    /// \brief Finds the rank of the predecessor of the specified key in the compressed trie.
    /// \param keys the keys that the compressed trie was constructed for
    /// \param num the number of keys
    /// \param x the key in question
    Result predecessor(const uint64_t* keys, const size_t num, const uint64_t x) const;
};

}} // namespace tdc::pred
