#pragma once

#include <utility>

namespace tdc {
namespace pred {

/// \brief The result of a predecessor or successor query.
struct Result {
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

}} // namespace tdc::pred
