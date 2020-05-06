#pragma once

#include <cstddef>

namespace tdc {

/// \brief Wrapper around an index accessor that accesses only every <em>k</em>-th item.
/// \tparam T the item type
template<typename T>
class SkipAccessor {
private:
    const T* m_data;
    size_t m_skip;
    size_t m_offset;

public:
    inline SkipAccessor() : m_data(nullptr, 0, 0) {
    }

    /// \brief Constructs a skip accessor.
    /// \param data a pointer to the underlying array
    /// \param skip the number of items to skip after each index
    /// \param offset the index of the first item
    inline SkipAccessor(const T* data, const size_t skip, const size_t offset = 0) : m_data(data), m_skip(skip), m_offset(offset) {
    }
    
    /// \brief Accesses an element.
    ///
    /// In reference to the constructor parameters, this will access element <tt>offset + i * skip</tt> from <tt>data</tt>.
    inline const T& operator[](const size_t i) const {
        return m_data[m_offset + i * m_skip];
    }
};

}; // namespace tdc
