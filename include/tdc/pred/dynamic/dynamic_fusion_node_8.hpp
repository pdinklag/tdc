#pragma once

#include <cstddef>
#include <cstdint>
#include <tdc/util/skip_accessor.hpp>

#include "../result.hpp"

namespace tdc {
namespace pred {
namespace dynamic {

class DynamicFusionNode8 {
public:
    /// \brief The maximum number of keys supported by this data structure.
    static constexpr size_t MAX_KEYS = 8;

private:
    size_t m_size;
    uint64_t m_key[MAX_KEYS], m_index;
    uint64_t m_mask, m_branch, m_free;
    uint8_t m_bkey;

    size_t find(const uint64_t key) const;
    size_t rank(const uint64_t key) const;
    bool used(const size_t j) const;

public:
    /// \brief Constructs an empty fusion node.
    DynamicFusionNode8();

    /// \brief Selects the key with the given rank.
    /// \param i the rank in question
    uint64_t select(const size_t i) const;

    /// \brief Inserts the specified key.
    /// \param key the key to insert
    void insert(const uint64_t key);

    /// \brief Removes the specified key.
    /// \param key the key to remove
    void remove(const uint64_t key);

    /// \brief Returns the current size of the fusion node.
    inline size_t size() const {
        return m_size;
    }

    // FIXME DEBUG
    void print() const;
};

}}} // namespace tdc::pred::dynamic
