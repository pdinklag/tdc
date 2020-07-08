#pragma once
#ifdef PLADS_FOUND

#include <cstddef>
#include <tdc/pred/result.hpp>
#include <plads/bit_vector_dynamic/bit_vector_dynamic_unique.hpp>

namespace tdc {
namespace pred {
namespace dynamic {

class DynamicRankSelect {
private:
    using DynamicBitVector = plads::dynamic::bit_vector_dynamic_unique;
    
    size_t m_size;
    DynamicBitVector m_dbv;

public:
    DynamicRankSelect();
    
    /// \brief Finds the \em value of the predecessor of the specified key in the trie.
    /// \param x the key in question
    Result predecessor(const uint64_t x) const;

    /// \brief Inserts the specified key.
    /// \param key the key to insert
    void insert(const uint64_t key);

    /// \brief Removes the specified key.
    /// \param key the key to remove
    /// \return whether the item was found and removed
    bool remove(const uint64_t key);
    
    /// \brief Returns the current size of the underlying octrie.
    inline size_t size() const {
        return m_size;
    }

};

}}} // namespace tdc::pred::dynamic

#endif
