#pragma once

#include <cassert>
#include <cstdint>
#include <tdc/pred/dynamic/buckets/buckets.hpp>
#include <tdc/pred/result.hpp>
#include <tdc/util/likely.hpp>
#include <unordered_map>
#include <utility>
#include <vector>
#include <robin_hood.h>

namespace tdc {
namespace pred {
namespace dynamic {

template <template <uint8_t> class t_bucket, uint8_t t_sampling>
class DynIndexMap {
 private:
  static_assert(t_sampling <= 32);

  // The bottom part of the data structure is either
  // a bit vector, std::array, or a hybrid.
  static constexpr size_t b_wordl = t_sampling;
  static constexpr size_t b_max = (1ULL << b_wordl) - 1;
  using bucket = t_bucket<t_sampling>;
  size_t m_size = 0;
  uint64_t m_min = std::numeric_limits<uint64_t>::max();  // stores the minimal key
  uint64_t m_max = std::numeric_limits<uint64_t>::min();  // stores the maximal key

  robin_hood::unordered_map<uint32_t, bucket> m_map;

  // return the x_wordl more significant bits of i
  inline uint64_t prefix(uint64_t i) const { return i >> b_wordl; }
  // return the b_wordl less significant bits
  inline uint64_t suffix(uint64_t i) const { return i & b_max; }

 public:
  DynIndexMap(){};
  DynIndexMap(const uint64_t* keys, const size_t num) {
    for (size_t i = 0; i < num; ++i) {
      insert(keys[i]);
    }
  }

  size_t size() const {
    return m_size;
  }

  void insert(uint64_t key) {
    assert(!predecessor(key).exists || predecessor(key).key != key);
    const uint64_t key_pre = prefix(key);
    const uint64_t key_suf = suffix(key);

    m_map[key_pre].set(key_suf);
    ++m_size;
    m_min = std::min(key, m_min);
    m_max = std::max(key, m_max);
    assert(predecessor(key).exists);
    assert(predecessor(key).key == key);
  }

  void remove(uint64_t key) {
    const uint64_t key_pre = prefix(key);
    const uint64_t key_suf = suffix(key);
    assert(predecessor(key).exists && predecessor(key).key == key);
    //start here
    bucket& b = m_map.at(key_pre);
    b.remove(key_suf);
    --m_size;

    if (b.size() == 0) {
      m_map.erase(key_pre);
      return;
    }

    if (m_size == 0) {
      m_min = std::numeric_limits<uint64_t>::max();
      m_max = std::numeric_limits<uint64_t>::min();
      robin_hood::unordered_map<uint32_t, bucket>().swap(m_map);
      return;
    }

    if (key == m_max) {
      assert(predecessor(key - 1).exists);
      m_max = predecessor(key - 1).key;
    }
    if (key == m_min) {
      uint64_t i = key_pre;
      while (m_map.find(i) == m_map.end()) {
        ++i;
      }
      m_min = (i << b_wordl) + m_map.at(i).get_min();
    }
  }

  KeyResult<uint64_t> predecessor(uint64_t key) const {
    if (tdc_unlikely(m_size == 0)) {
      return {false, 0};
    }
    if (tdc_unlikely(key < m_min)) {
      return {false, 1};
    }
    if (tdc_unlikely(key >= m_max)) {
      return {true, m_max};
    }
    uint64_t key_pre = prefix(key);
    auto p = m_map.find(key_pre);
    if (p != m_map.end()) {
      auto r = p->second.predecessor(suffix(key));
      if (r.exists) {
        return {true, (key_pre << b_wordl) + r.key};
      }
    }
    do {
      --key_pre;
      p = m_map.find(key_pre);
    } while (p == m_map.end());
    auto r = p->second.predecessor(b_max);
    assert(r.exists);
    return {true, (key_pre << b_wordl) + r.key};
  }
};
}  // namespace dynamic
}  // namespace pred
}  // namespace tdc
