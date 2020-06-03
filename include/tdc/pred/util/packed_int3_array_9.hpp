#pragma once

#include <cstdint>

namespace tdc {
namespace pred {

/// \brief An array of eight 3-bit-integers and one byte packed into a 32-bit word.
struct PackedInt3Array9 {
    /// \brief The stored integers.
    unsigned int x0 : 3;
    unsigned int x1 : 3;
    unsigned int x2 : 3;
    unsigned int x3 : 3;
    unsigned int x4 : 3;
    unsigned int x5 : 3;
    unsigned int x6 : 3;
    unsigned int x7 : 3;
    unsigned int x8 : 8;
    
    struct Int3Ref {
        PackedInt3Array9& arr;
        const size_t i;
        
        inline operator uint8_t() const {
            switch(i) {
                case 0: return arr.x0;
                case 1: return arr.x1;
                case 2: return arr.x2;
                case 3: return arr.x3;
                case 4: return arr.x4;
                case 5: return arr.x5;
                case 6: return arr.x6;
                case 7: return arr.x7;
                case 8: return arr.x8;
                default: return UINT8_MAX;
            }
        }
        
        inline Int3Ref& operator=(const uint8_t x) {
            switch(i) {
                case 0: arr.x0 = x; break;
                case 1: arr.x1 = x; break;
                case 2: arr.x2 = x; break;
                case 3: arr.x3 = x; break;
                case 4: arr.x4 = x; break;
                case 5: arr.x5 = x; break;
                case 6: arr.x6 = x; break;
                case 7: arr.x7 = x; break;
                case 8: arr.x8 = x; break;
            }
            return *this;
        }
        
        inline Int3Ref& operator=(const Int3Ref& x) {
            *this = (uint8_t)x;
            return *this;
        }
    };
    
    inline uint8_t operator[](const size_t i) const {
        switch(i) {
            case 0: return x0;
            case 1: return x1;
            case 2: return x2;
            case 3: return x3;
            case 4: return x4;
            case 5: return x5;
            case 6: return x6;
            case 7: return x7;
            case 8: return x8;
            default: return UINT8_MAX;
        }
    }

    inline Int3Ref operator[](const size_t i) {
        return { *this, i };
    }
    
    /// \brief Default constructor.
    inline PackedInt3Array9() {
    }
} __attribute__((__packed__));

// sanity check
static_assert(sizeof(PackedInt3Array9) == 4);

}} // namespace tdc::pred
