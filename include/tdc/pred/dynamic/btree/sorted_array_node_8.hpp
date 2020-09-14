#pragma once

#include <cstddef>
#include <cstdint>

#include <tdc/pred/result.hpp>

namespace tdc {
namespace pred {
namespace dynamic {

using Result = ::tdc::pred::Result;

class SortedArrayNode8 {
private:
    uint64_t m_keys[8];
    uint8_t m_size;

public:
    /// \brief Constructs an empty fusion node.
    SortedArrayNode8();

    SortedArrayNode8(const SortedArrayNode8&) = default;
    SortedArrayNode8(SortedArrayNode8&&) = default;

    SortedArrayNode8& operator=(const SortedArrayNode8&) = default;
    SortedArrayNode8& operator=(SortedArrayNode8&&) = default;

    /// \brief Accesses the element with given rank
    /// \param i the rank in question    
    inline uint64_t operator[](const size_t i) const {
        return m_keys[i];
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
} __attribute__((__packed__));;

}}} // namespace tdc::pred::dynamic
