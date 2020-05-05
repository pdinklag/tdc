#pragma once

#include <cstdint>
#include <cstddef>

#include "result.hpp"

namespace tdc {
namespace pred {

class BinarySearchHybrid {
private:
    uint64_t m_min, m_max;
    size_t m_cache_num;

public:
    inline BinarySearchHybrid() : m_min(0), m_max(UINT64_MAX), m_cache_num(0) {
    }

    BinarySearchHybrid(const uint64_t* keys, const size_t num, const size_t cache_num = 512ULL / sizeof(uint64_t));

    BinarySearchHybrid(const BinarySearchHybrid& other) = default;
    BinarySearchHybrid(BinarySearchHybrid&& other) = default;
    BinarySearchHybrid& operator=(const BinarySearchHybrid& other) = default;
    BinarySearchHybrid& operator=(BinarySearchHybrid&& other) = default;

    Result predecessor_seeded(const uint64_t* keys, size_t p, size_t q, const uint64_t x) const;
    Result predecessor(const uint64_t* keys, const size_t num, const uint64_t x) const;
};

}} // namespace tdc::pred
