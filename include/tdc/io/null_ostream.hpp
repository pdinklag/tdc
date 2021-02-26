#pragma once

#include <iostream>

namespace tdc {
namespace io {

/// \brief An output stream that behaves like /dev/null.
class NullOStream : public std::ostream {
public:
    inline NullOStream() : std::ostream(nullptr) {
    }
    
    NullOStream(const NullOStream&) : std::ostream(nullptr) {
    }
};

}} // namespace tdc::io

template<typename T>
const tdc::io::NullOStream& operator<<(tdc::io::NullOStream& os, const T& value) { 
    return os;
}
