#pragma once

#include <utility>

namespace tdc {
namespace pred {

/// \brief The result of a predecessor or successor query, wrapping the position of the located key.
struct PosResult {
    /// \brief Whether the predecessor or successor exists.
    bool exists;
    
    /// \brief The position of the predecessor or successor, only meaningful if \ref exists is \c true.
    size_t pos;
    
    inline operator bool() const {
        return exists;
    }

    inline operator size_t() const {
        return pos;
    }
};

/// \brief The result of a predecessor or successor query, wrapping the located key.
/// \tparam key_t the key type
template<typename key_t>
struct KeyResult {
    /// \brief Whether the predecessor or successor exists.
    bool exists;
    
    /// \brief The position of the predecessor or successor, only meaningful if \ref exists is \c true.
    key_t key;
    
    inline operator bool() const {
        return exists;
    }

    inline operator key_t() const {
        return key;
    }
};

}} // namespace tdc::pred
