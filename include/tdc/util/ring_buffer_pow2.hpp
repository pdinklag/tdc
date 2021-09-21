#pragma once

#include <cstddef>

#include <tdc/math/bit_mask.hpp>
#include <tdc/util/index.hpp>

namespace tdc {

/// \brief Ring buffer of capacity a power of two.
template<typename Item>
class RingBufferPow2 {
private:
    size_t mask_;

    Item*  items_;
    size_t size_;
    size_t max_size_;

    size_t start_;
    size_t end_;

    inline size_t clamp(const size_t i) {
        return i & mask_;
    }

public:
    RingBufferPow2(size_t exp)
        : mask_(math::bit_mask<size_t>(exp)),
          size_(0),
          max_size_(1ULL << exp),
          start_(0),
          end_(0) {

        items_ = new Item[1ULL << exp];
    }

    ~RingBufferPow2() {
        delete[] items_;
    }

    void clear() {
        start_ = 0;
        end_ = 0;
        size_ = 0;
    }

    void push_back(const Item& item) {
        assert(size_ < max_size_);
        items_[end_] = item;
        end_ = clamp(end_ + 1);
        ++size_;
    }

    void pop_front() {
        assert(size_ > 0);
        start_ = clamp(start_ + 1);
        --size_;
    }

    Item& operator[](const size_t i) {
        return items_[clamp(start_ + i)];
    }

    const Item& operator[](const size_t i) const {
        return items_[clamp(start_ + i)];
    }

    Item& front() {
        return items_[start_];
    }

    const Item& front() const {
        return items_[start_];
    }

    size_t size() const {
        return size_;
    }

    size_t max_size() const {
        return max_size_;
    }
};

}
