#pragma once

#include <cstdint>
#include <cstddef>

#include "result.hpp"

namespace tdc {
namespace pred {

class BinarySearch {
private:
    uint64_t m_min, m_max;

public:
    inline BinarySearch() : m_min(0), m_max(UINT64_MAX) {
    }

    BinarySearch(const uint64_t* keys, const size_t num);

    BinarySearch(const BinarySearch& other) = default;
    BinarySearch(BinarySearch&& other) = default;
    BinarySearch& operator=(const BinarySearch& other) = default;
    BinarySearch& operator=(BinarySearch&& other) = default;

    Result predecessor(const uint64_t* keys, const size_t num, const uint64_t x) const;
};

}} // namespace tdc::pred
