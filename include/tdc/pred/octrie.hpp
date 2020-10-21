#pragma once

#include "fusion_node.hpp"

#include <tdc/math/idiv.hpp>
#include <tdc/math/ilog2.hpp>

#include <vector>

namespace tdc {
namespace pred {

/// \brief Predecessor search in an octree of \ref CompressedTrie8 instances.
class Octrie {
protected:
    inline static constexpr size_t log8_ceil(const size_t x) {
        using namespace tdc::math;
        return idiv_ceil(ilog2_ceil(x), 3); // log8(x) = log2(x) / log2(8)
    }

    inline static constexpr size_t eight_to_the(const size_t x) {
        return 1ULL << (3 * x); // 8^x = 2^3x
    }

    inline static constexpr size_t octree_size(const size_t height) {
        return (eight_to_the(height) - 1) / 7;
    }

    struct octree_level_t {
        size_t first_node;
        std::vector<FusionNode<>> nodes;
    };

    std::vector<octree_level_t> m_octree;
    FusionNode<>* m_root;
    
    size_t m_octree_size_ub;
    size_t m_height;
    size_t m_full_octree_height;
    
    /// \brief Constructs an octrie for the given keys and the given maximum height.
    /// \param keys a pointer to the keys, that must be in ascending order
    /// \param num the number of keys
    /// \param height the maximum height of the octrie
    Octrie(const uint64_t* keys, const size_t num, const size_t max_height);

public:
    /// \brief Constructs an empty octrie.
    inline Octrie() : m_root(nullptr), m_octree_size_ub(0), m_height(0) {
    }

    /// \brief Constructs an octrie for the given keys.
    /// \param keys a pointer to the keys, that must be in ascending order
    /// \param num the number of keys
    Octrie(const uint64_t* keys, const size_t num);
    
    Octrie(const Octrie& other) = default;
    Octrie(Octrie&& other) = default;
    Octrie& operator=(const Octrie& other) = default;
    Octrie& operator=(Octrie&& other) = default;
    
    /// \brief Finds the rank of the predecessor of the specified key.
    /// \param keys the keys that the compressed trie was constructed for
    /// \param num the number of keys
    /// \param x the key in question
    PosResult predecessor(const uint64_t* keys, const size_t num, const uint64_t x) const;
};

}} // namespace tdc::pred
