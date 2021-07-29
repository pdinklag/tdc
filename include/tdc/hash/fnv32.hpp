#pragma once

#include <cstdint>

#include <tdc/hash/byte.hpp>

namespace tdc {
namespace hash {

class FNV32 {
private:
    static constexpr uint32_t offset_ = 0x811c9dc5;
    static constexpr uint32_t prime_  = 0x01000193;
    
public:
    inline uint32_t operator()(const Byte* s, const size_t n) const {
        uint32_t h = offset_;
        for(size_t i = 0; i < n; i++) {
            // FNV-1a
            h ^= s[i];
            h *= prime_;
        }
        return h;
    }
};

}}
