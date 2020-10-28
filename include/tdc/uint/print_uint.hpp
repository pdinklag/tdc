#pragma once

#include <ostream>

namespace tdc {

/// \cond INTERNAL
template<typename T>
inline std::ostream& print_uint(std::ostream& out, T v) {
    static_assert(sizeof(T) <= 128); // support up to 1024-bit values

    if(v == 0) return out << '0';
    
    auto flags = out.flags();
    
    constexpr size_t max_digits = 512; // the largest 1024-bit value has 343 octal digits
    char buf[max_digits] = { 0 }; 
    char* p = buf + max_digits - 1;
    
    T base = 0;
    const char* digits = nullptr;
    
    if(flags & std::ios_base::dec) {
        base = 10;
        digits = "0123456789";
    } else if(flags & std::ios_base::oct) {
        base = 8;
        digits = "0123456789";
    } else if(flags & std::ios_base::hex) {
        base = 16;
        digits = (flags & std::ios_base::uppercase) ? "0123456789ABCDEF" : "0123456789abcdef";
    }
    
    if(base) {
        while(v) {
            *--p = digits[(uint64_t)(v % base)];
            v /= base;
        }
    }
    
    out << p;
    return out;
}
/// \endcond

}
