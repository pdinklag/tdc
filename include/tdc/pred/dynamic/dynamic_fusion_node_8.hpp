#pragma once

#include <cstddef>
#include <cstdint>

#include <tdc/pred/result.hpp>
#include <tdc/pred/util/packed_int3_array_9.hpp>
#include <tdc/util/skip_accessor.hpp>

namespace tdc {
namespace pred {
namespace dynamic {

using Result = ::tdc::pred::Result;

class DynamicFusionNode8 {
private:
    uint64_t m_key[8];
    uint64_t m_mask, m_branch, m_free;
    PackedInt3Array9 m_index;
    uint8_t m_bkey;
    uint8_t m_size;

    size_t find(const uint64_t key) const;
    size_t rank(const uint64_t key) const;
    bool used(const size_t j) const;

public:
    /// \brief Constructs an empty fusion node.
    DynamicFusionNode8();

    DynamicFusionNode8(const DynamicFusionNode8&) = default;
    DynamicFusionNode8(DynamicFusionNode8&&) = default;

    DynamicFusionNode8& operator=(const DynamicFusionNode8&) = default;
    DynamicFusionNode8& operator=(DynamicFusionNode8&&) = default;

    /// \brief Selects the key with the given rank.
    /// \param i the rank in question
    uint64_t select(const size_t i) const;

    /// \brief Convenience operator for \ref select.
    /// \param i the rank in question    
    inline uint64_t operator[](const size_t i) const {
        return select(i);
    }
    
    /// \brief Finds the rank of the predecessor of the specified key in the node.
    /// \param x the key in question
    Result predecessor(const uint64_t x) const;

    /// \brief Inserts the specified key.
    /// \param key the key to insert
    void insert(const uint64_t key);

    /// \brief Removes the specified key.
    /// \param key the key to remove
    /// \return whether the item was found and removed
    bool remove(const uint64_t key);

    /// \brief Returns the current size of the fusion node.
    inline size_t size() const {
        return m_size;
    }

    // FIXME DEBUG
    void print() const;
};

}}} // namespace tdc::pred::dynamic
