#pragma once

#include <cstddef>
#include <cstdint>
#include <tdc/util/skip_accessor.hpp>

#include "result.hpp"

namespace tdc {
namespace pred {

/// \brief A compressed trie that can solve predecessor queries for up to 8 64-bit keys using only 128 bits.
class FusionNode8 {
private:
    uint64_t m_mask, m_branch, m_free;

public:
    /// \brief Constructs an empty compressed trie.
    FusionNode8();

    /// \brief Constructs a compressed trie for the given keys.
    ///
    /// Note that the keys are \em not stored in the trie.
    /// This is to keep this data structure as small as possible.
    /// The keys will, howver, be needed for predecessor lookups and thus need to be managed elsewhere.
    ///
    /// \param keys a pointer to the keys, that must be in ascending order
    /// \param num the number of keys, must be at most \ref MAX_KEYS
    FusionNode8(const uint64_t* keys, const size_t num);
    
    /// \brief Constructs a compressed trie for the given keys.
    ///
    /// Note that the keys are \em not stored in the trie.
    /// This is to keep this data structure as small as possible.
    /// The keys will, howver, be needed for predecessor lookups and thus need to be managed elsewhere.
    ///
    /// \param keys a pointer to the keys, that must be in ascending order
    /// \param num the number of keys, must be at most \ref MAX_KEYS
    FusionNode8(const SkipAccessor<uint64_t>& keys, const size_t num);

    /// \brief Finds the rank of the predecessor of the specified key in the compressed trie.
    /// \param keys the keys that the compressed trie was constructed for
    /// \param x the key in question
    Result predecessor(const uint64_t* keys, const uint64_t x) const;
    
    /// \brief Finds the rank of the predecessor of the specified key in the compressed trie.
    /// \param keys the keys that the compressed trie was constructed for
    /// \param x the key in question
    Result predecessor(const SkipAccessor<uint64_t>& keys, const uint64_t x) const;
};

}} // namespace tdc::pred
