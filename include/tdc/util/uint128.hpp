#pragma once

#include <ostream>

#ifdef __SIZEOF_INT128__

using uint128_t = __uint128_t;

#else

static_assert(false, "__uint128_t is not available");

#endif

namespace std {

inline ostream& operator<<(ostream& out, uint128_t v) {
    auto flags = out.flags();
    
    constexpr size_t max_digits = 45; // the largest 128-bit value has 44 octal digits - add one for trailing zero
    char buf[max_digits] = { 0 }; 
    char* p = buf + max_digits - 1;
    
    uint128_t base = 0;
    const char* digits = nullptr;
    
    if(flags & ios_base::dec) {
        base = 10;
        digits = "0123456789";
    } else if(flags & ios_base::oct) {
        base = 8;
        digits = "0123456789";
    } else if(flags & ios_base::hex) {
        base = 16;
        digits = (flags & ios_base::uppercase) ? "0123456789ABCDEF" : "0123456789abcdef";
    }
    
    if(base) {
        while(v) {
            *--p = digits[v % base];
            v /= base;
        }
    }
    
    out << p;
    return out;
}

#include <climits>
#define UINT128_MAX std::numeric_limits<uint128_t>::max()

}
