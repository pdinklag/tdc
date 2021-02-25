#pragma once

#include <cstddef>
#include <cstdint>

#include <tdc/pred/fusion_node_internals.hpp>
#include <tdc/util/concepts.hpp>
#include <tdc/util/skip_accessor.hpp>

#include "result.hpp"

namespace tdc {
namespace pred {

/// \brief A compressed trie that can solve predecessor queries for up to 8 64-bit keys using only 128 bits.
/// \tparam the key type
/// \tparam the maximum number of keys
template<std::totally_ordered key_t = uint64_t, size_t m_max_keys = 8>
class FusionNode {
private:
    using Internals = internal::FusionNodeInternals<key_t, m_max_keys, false>;
    
    using mask_t = typename Internals::mask_t;
    using matrix_t = typename Internals::matrix_t;

    mask_t   m_mask;
    matrix_t m_branch, m_free;

public:
    /// \brief Constructs an empty compressed trie.
    FusionNode(): m_mask(0) {
    }

    /// \brief Constructs a compressed trie for the given keys.
    ///
    /// Note that the keys are \em not stored in the trie.
    /// This is to keep this data structure as small as possible.
    /// The keys will, howver, be needed for predecessor lookups and thus need to be managed elsewhere.
    ///
    /// \param keys a pointer to the keys, that must be in ascending order
    /// \param num the number of keys, must be at most \ref MAX_KEYS
    template<IndexAccessTo<key_t> keyarray_t>
    FusionNode(const keyarray_t& keys, const size_t num) {
        auto fnode8 = Internals::template construct<key_t>(keys, num);
        m_mask = std::get<0>(fnode8);
        m_branch = std::get<1>(fnode8);
        m_free = std::get<2>(fnode8);
    }

    /// \brief Finds the rank of the predecessor of the specified key in the compressed trie.
    /// \param keys the keys that the compressed trie was constructed for
    /// \param x the key in question
    template<IndexAccessTo<key_t> keyarray_t>
    PosResult predecessor(const keyarray_t& keys, const key_t x) const {
        return Internals::predecessor(keys, x, m_mask, m_branch, m_free);
    }
};

}} // namespace tdc::pred
