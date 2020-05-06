#pragma once

#include <tdc/util/uint40.hpp>

namespace tdc {
namespace vec {

/// \brief Proxy for reading and writing a single item in a vector.
/// \tparam vector_t the vector type
/// \tparam item_t the item type
template<typename vector_t, typename item_t>
class ItemRef {
protected:
    vector_t* m_vec;
    size_t m_i;
    
public:
    /// \brief Main constructor.
    /// \param vec the vector this proxy belongs to.
    /// \param i the number of the referred item.
    inline ItemRef(vector_t& vec, size_t i) : m_vec(&vec), m_i(i) {
    }

    /// \brief Reads the referred item.
    inline operator item_t() const {
        return m_vec->get(m_i);
    }

    /// \brief Writes the referred integer.
    /// \param v the value to write
    inline void operator=(const item_t v) {
        m_vec->set(m_i, v);
    }
};

/// \brief Specialization of \ref ItemRef for \ref uint40_t items.
///
/// The reason this exists is to resolve issues concerning automatic casts.
template<typename vector_t>
class ItemRef<vector_t, uint40_t> : public ItemRef<vector_t, uint64_t> {
private:
    using base_t = ItemRef<vector_t, uint64_t>;
    
public:
    using base_t::ItemRef;

    /// \brief Reads the referred item.
    inline operator uint40_t() const {
        return base_t::m_vec->get(base_t::m_i);
    }

    /// \brief Reads the referred item.
    inline operator uint64_t() const {
        return base_t::m_vec->get(base_t::m_i);
    }

    /// \brief Writes the referred integer.
    /// \param v the value to write
    inline void operator=(const uint40_t v) {
        base_t::m_vec->set(base_t::m_i, v);
    }

    /// \brief Writes the referred integer.
    /// \param v the value to write
    inline void operator=(const uint64_t v) {
        base_t::m_vec->set(base_t::m_i, v);
    }
};

}} // namespace tdc::vec
