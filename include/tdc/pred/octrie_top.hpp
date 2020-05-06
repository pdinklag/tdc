#pragma once

#include "binary_search_hybrid.hpp"
#include "octrie.hpp"

namespace tdc {
namespace pred {

/// \brief Predecessor search in the top levels of an \ref Octrie, followed by linear search within blocks of 64 elements.
class OctrieTop : public Octrie {
private:
    size_t m_full_octree_size_ub;
    size_t m_cut_levels;
    size_t m_search_interval;
    
    BinarySearchHybrid m_search;

public:
    /// \brief Constructs an empty octrie.
    inline OctrieTop() : Octrie() {
    }

    /// \brief Constructs an octrie for the given keys.
    /// \param keys a pointer to the keys, that must be in ascending order
    /// \param num the number of keys
    /// \param cut_levels the number of bottom levels to cut off
    OctrieTop(const uint64_t* keys, const size_t num, const size_t cut_levels);
    
    OctrieTop(const OctrieTop& other) = default;
    OctrieTop(OctrieTop&& other) = default;
    OctrieTop& operator=(const OctrieTop& other) = default;
    OctrieTop& operator=(OctrieTop&& other) = default;
    
    /// \brief Finds the rank of the predecessor of the specified key.
    /// \param keys the keys that the compressed trie was constructed for
    /// \param num the number of keys
    /// \param x the key in question
    Result predecessor(const uint64_t* keys, const size_t num, const uint64_t x) const;
};

}} // namespace
