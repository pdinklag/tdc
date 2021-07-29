#pragma once

#include <array>
#include <cstdint>

#include <tdc/hash/byte.hpp>

namespace tdc {
namespace hash {

using CRC32Table = std::array<uint32_t, 256>;

constexpr CRC32Table build_crc32_table() {
    CRC32Table table;
    for(uint32_t i = 0; i < 256; i++) {
        uint32_t byte = i;
        uint32_t crc = 0;
        for(size_t j=0; j < 8; j++) {
            const uint32_t b = (byte ^ crc) & 1;
            crc >>= 1;
            if(b) crc = crc ^ 0xEDB88320;
            byte >>= 1;
        }
        table[i] = crc;
    }
    return table;
}

class CRC32 {
private:
    static constexpr CRC32Table table_ = build_crc32_table();

public:
    inline uint32_t operator()(const Byte* s, const size_t n) const {
        uint32_t crc = 0xFFFFFFFF;
        for(size_t i = 0; i < n; i++) {
            const Byte byte = s[i];
            const uint32_t t = (byte ^ crc) & 0xFF;
            crc = (crc >> 8) ^ table_[t];
        }
        return ~crc;
    }
};

}}
