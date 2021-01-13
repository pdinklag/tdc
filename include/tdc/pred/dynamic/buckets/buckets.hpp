#pragma once

#include <cassert>
#include <limits>
#include <type_traits>

#include <tdc/math/bit_mask.hpp>
#include <tdc/vec/fixed_width_int_vector.hpp>

namespace tdc {
namespace pred {
namespace dynamic {

template<uint8_t b_wordl>
struct bucket_base {
    static_assert(b_wordl <= 32, "bucket sizes beyond 2^32 not supported");
    using suffix_t = typename std::conditional<b_wordl <= 16, uint16_t, uint32_t>::type;
    using list_t = typename tdc::vec::FixedWidthIntVector<b_wordl>::builder_type;
    
    static constexpr uint64_t SUFFIX_MAX = tdc::math::bit_mask<uint64_t>(b_wordl);
    static constexpr uint64_t MAX_NUM = 1ULL << b_wordl;
    
    inline static constexpr uint64_t prefix(uint64_t i) {
        return i >> b_wordl;
    }
    
    inline static constexpr uint64_t suffix(uint64_t i) {
        return i & SUFFIX_MAX;
    }

    template<typename key_t>
    inline static constexpr size_t hybrid_threshold() {
        // in the hybrid data structures, we want to use an unsorted list when it is smaller than a bit vector of length 2^t
        // this is the case while b'*(lg u) < 2^t <=> b' < 2^t / (lg u)
        return MAX_NUM / std::numeric_limits<key_t>::digits;
    }
};

// This is a bucket that holds a bit vector.
template <typename key_t, uint8_t b_wordl>
struct bucket_bv : bucket_base<b_wordl> {
  using base = bucket_base<b_wordl>;
  using suffix_t = typename base::suffix_t;
    
  suffix_t m_size = 0;
  const uint64_t m_prefix;
  uint64_t m_prev_pred = 0;
  bucket_bv *m_next_b = nullptr;  // next bucket
  std::bitset<base::MAX_NUM> m_bits;

  bucket_bv(uint64_t prefix, uint64_t prev_pred, bucket_bv *next_b, uint64_t suf)
      : m_prefix(prefix), m_prev_pred(prev_pred), m_next_b(next_b) {
    set(suf);
  }

  uint64_t get_min() {
    assert(m_size > 0);
    size_t i = 0;
    while (!m_bits[i]) {
      ++i;
    }
    return (m_prefix << b_wordl) + i;
  }

  void set(const uint64_t suf) {
    m_bits[suf] = true;
    ++m_size;
    if (tdc_likely(m_next_b != nullptr)) {
      m_next_b->m_prev_pred = std::max((m_prefix << b_wordl) + suf, m_next_b->m_prev_pred);
    }
  }

  // deletes a key and updates the bottom level of the data structure
  // it can not update the top level
  void remove(uint64_t suf) {
    m_bits[suf] = false;
    --m_size;
    // here we update next_b->prev_pred
    if (tdc_likely(m_next_b != nullptr)) {
      if (m_next_b->m_prev_pred == (m_prefix << b_wordl) + suf) {
        m_next_b->m_prev_pred = predecessor((m_prefix << b_wordl) + suf);
      }
    }
  }

  size_t size() {
    return m_size;
  }
  uint64_t predecessor(int64_t key) const {
    key = base::prefix(key) != m_prefix ? base::SUFFIX_MAX : base::suffix(key);
    for (; key >= 0; --key) {
      if (m_bits[key]) {
        return (m_prefix << b_wordl) + key;
      }
    }
    return m_prev_pred;
  }
};

// This is a bucket that holds an std::vector.
template <typename key_t, uint8_t b_wordl>
struct bucket_list : bucket_base<b_wordl> {
  using base = bucket_base<b_wordl>;
  using suffix_t = typename base::suffix_t;
  using list_t = typename base::list_t;
    
  const uint64_t m_prefix;
  uint64_t m_prev_pred = 0;
  bucket_list *m_next_b = nullptr;
  list_t m_list;

  bucket_list(uint64_t prefix, uint64_t prev_pred, bucket_list *next_b, suffix_t suf)
      : m_prefix(prefix), m_prev_pred(prev_pred), m_next_b(next_b) {
    set(suf);
  }

  uint64_t get_min() {
    assert(m_list.size() > 0);
    return (m_prefix << b_wordl) + *std::min_element(std::begin(m_list), std::end(m_list));
  }

  void set(uint64_t suf) {
    m_list.push_back(suf);
    if (tdc_likely(m_next_b != nullptr)) {
      m_next_b->m_prev_pred = std::max((m_prefix << b_wordl) + suf, m_next_b->m_prev_pred);
    }
  }

  void remove(uint64_t suf) {
    auto p = std::find(m_list.begin(), m_list.end(), suf);
    assert(p != m_list.end());
    *p = (suffix_t)m_list.back();
    m_list.pop_back();
    // here we update next_b->prev_pred
    if (tdc_likely(m_next_b != nullptr)) {
      if (m_next_b->m_prev_pred == (m_prefix << b_wordl) + suf) {
        m_next_b->m_prev_pred = predecessor((m_prefix << b_wordl) + suf);
      }
    }
  }

  size_t size() {
    return m_list.size();
  }

  uint64_t predecessor(int64_t key) const {
    key = base::prefix(key) != m_prefix ? base::SUFFIX_MAX : base::suffix(key);

    auto p = std::find_if(m_list.begin(), m_list.end(), [key](const uint64_t x) { return x <= key; });
    if (p == m_list.end()) {
      return m_prev_pred;
    }
    suffix_t max_pred = *p;
    for (; p != m_list.end(); ++p) {
      if ((*p) <= key) {
        max_pred = std::max((suffix_t)*p, max_pred);
      }
    }
    return (m_prefix << b_wordl) + max_pred;
  }
};

// This is a bucket that either holds an std::vector or a bit_vector depending
// on fill rate
template <typename key_t, uint8_t b_wordl>
struct bucket_hybrid : bucket_base<b_wordl> {
  using base = bucket_base<b_wordl>;
  using suffix_t = typename base::suffix_t;
  using list_t = typename base::list_t;
    
  static constexpr size_t upper_threshhold = base::template hybrid_threshold<key_t>();
  static constexpr size_t lower_threshhold = base::template hybrid_threshold<key_t>() / 2;
  suffix_t m_size = 0;
  const uint64_t m_prefix;
  uint64_t m_prev_pred = 0;
  bucket_hybrid *m_next_b = nullptr;

  bool m_is_list = true;
  list_t m_list;
  std::bitset<base::MAX_NUM>* m_bits = nullptr;

  bucket_hybrid(uint64_t prefix, uint64_t prev_pred, bucket_hybrid *next_b, suffix_t suf)
      : m_prefix(prefix), m_prev_pred(prev_pred), m_next_b(next_b) {
    set(suf);
  }

  ~bucket_hybrid() {
    delete m_bits;
  }
  uint64_t get_min() {
    assert(m_size > 0);
    if (m_is_list) {
      return (m_prefix << b_wordl) + *std::min_element(std::begin(m_list), std::end(m_list));
    } else {
      size_t i = 0;
      while (!(*m_bits)[i]) {
        ++i;
      }
      return (m_prefix << b_wordl) + i;
    }
  }

  void set(const uint64_t suf) {
    if (m_is_list) {
      m_list.push_back(suf);
    } else {
      (*m_bits)[suf] = true;
    }

    ++m_size;
    if (tdc_likely(m_next_b != nullptr)) {
      m_next_b->m_prev_pred = std::max((m_prefix << b_wordl) + suf, m_next_b->m_prev_pred);
    }
    if (m_is_list && (m_size > upper_threshhold)) {
      rebuild();
    }
  }

  void remove(const uint64_t suf) {
    if (m_is_list) {
      auto p = std::find(m_list.begin(), m_list.end(), suf);
      assert(p != m_list.end());
      *p = (suffix_t)m_list.back();
      m_list.pop_back();
    } else {
      (*m_bits)[suf] = false;
    }

    --m_size;
    // here we update next_b->prev_pred
    if (tdc_likely(m_next_b != nullptr)) {
      if (m_next_b->m_prev_pred == (m_prefix << b_wordl) + suf) {
        m_next_b->m_prev_pred = predecessor((m_prefix << b_wordl) + suf);
      }
    }
    if (!m_is_list && (m_size < lower_threshhold)) {
      rebuild();
    }
  }

  size_t size() {
    return m_size;
  }

  uint64_t predecessor(int64_t key) const {
    key = base::prefix(key) != m_prefix ? base::SUFFIX_MAX : base::suffix(key);
    if (m_is_list) {
      auto p = std::find_if(m_list.begin(), m_list.end(), [key](const uint64_t x) { return x <= key; });
      if (p == m_list.end()) {
        return m_prev_pred;
      }
      suffix_t max_pred = *p;
      for (; p != m_list.end(); ++p) {
        if ((*p) <= key) {
          max_pred = std::max((suffix_t)*p, max_pred);
        }
      }
      return (m_prefix << b_wordl) + max_pred;
    } else {
      for (; key >= 0; --key) {
        if ((*m_bits)[key]) {
          return (m_prefix << b_wordl) + key;
        }
      }
      return m_prev_pred;
    }
  }

  void
  rebuild() {
    if (m_is_list) {
      m_is_list = false;
      m_size = 0;
      m_bits = new std::bitset<base::MAX_NUM>; //std::vector<bool>(1ULL << b_wordl);
      for (suffix_t key : m_list) {
        set(key);
      }
      // this frees the memory allocated by the vector
      list_t().swap(m_list);
    } else {
      m_is_list = true;
      m_size = 0;
      for (size_t i = 0; i < base::MAX_NUM; ++i) {
        if ((*m_bits)[i]) {
          set(i);
        }
      }
      delete m_bits;
      m_bits = nullptr;
    }
  }
};

template <typename key_t, uint8_t b_wordl>
struct map_bucket_bv : bucket_base<b_wordl> {
  using base = bucket_base<b_wordl>;
  using suffix_t = typename base::suffix_t;
    
  suffix_t m_size = 0;
  std::bitset<base::MAX_NUM> m_bits;

  map_bucket_bv() {
  }

  map_bucket_bv(uint64_t suf) {
    set(suf);
  }

  uint64_t get_min() {
    assert(m_size > 0);
    size_t suf = 0;
    while (!m_bits[suf]) {
      ++suf;
    }
    return suf;
  }
  void set(const uint64_t suf) {
    m_bits[suf] = true;
    ++m_size;
  }

  void remove(uint64_t suf) {
    m_bits[suf] = false;
    --m_size;
  }

  size_t size() {
    return m_size;
  }

  KeyResult<uint64_t> predecessor(int64_t suf) const {
    for (; suf >= 0; --suf) {
      if (m_bits[suf]) {
        return {true, static_cast<uint64_t>(suf)};
      }
    }
    return {false, 0};
  }
};

// This is a bucket that holds an std::vector.
template <typename key_t, uint8_t b_wordl>
struct map_bucket_list : bucket_base<b_wordl> {
  using base = bucket_base<b_wordl>;
  using suffix_t = typename base::suffix_t;
  using list_t = typename base::list_t;

  list_t m_list;

  map_bucket_list() {
  }

  map_bucket_list(uint64_t suf) {
    set(suf);
  }

  uint64_t get_min() {
    assert(m_list.size() > 0);
    return *std::min_element(std::begin(m_list), std::end(m_list));
  }

  void set(uint64_t suf) {
    m_list.push_back(suf);
  }

  void remove(uint64_t suf) {
    auto p = std::find(m_list.begin(), m_list.end(), suf);
    assert(p != m_list.end());
    *p = (suffix_t)m_list.back();
    m_list.pop_back();
  }

  size_t size() {
    return m_list.size();
  }

  KeyResult<uint64_t> predecessor(int64_t suf) const {
    auto p = std::find_if(m_list.begin(), m_list.end(), [suf](const uint64_t x) { return x <= suf; });
    if (p == m_list.end()) {
      return {false, 0};
    }
    suffix_t max_pred = *p;
    for (; p != m_list.end(); ++p) {
      if ((*p) <= suf) {
        max_pred = std::max((suffix_t)*p, max_pred);
      }
    }
    return {true, max_pred};
  }
};

// This is a bucket that either holds an std::vector or a bit_vector depending
// on fill rate
template <typename key_t, uint8_t b_wordl>
struct map_bucket_hybrid : bucket_base<b_wordl> {
  using base = bucket_base<b_wordl>;
  using suffix_t = typename base::suffix_t;
  using list_t = typename base::list_t;
    
  static constexpr size_t upper_threshhold = base::template hybrid_threshold<key_t>();
  static constexpr size_t lower_threshhold = base::template hybrid_threshold<key_t>() / 2;
  suffix_t m_size = 0;
  bool m_is_list = true;
  list_t m_list;
  std::bitset<base::MAX_NUM> *m_bits = nullptr;

  map_bucket_hybrid() {
  }

  map_bucket_hybrid(suffix_t suf) {
    set(suf);
  }

  ~map_bucket_hybrid() {
    delete m_bits;
  }

  uint64_t get_min() {
    assert(m_size > 0);
    if (m_is_list) {
      return *std::min_element(std::begin(m_list), std::end(m_list));
    } else {
      size_t suf = 0;
      while (!(*m_bits)[suf]) {
        ++suf;
      }
      return suf;
    }
  }
  void set(const uint64_t suf) {
    if (m_is_list) {
      m_list.push_back(suf);
    } else {
      (*m_bits)[suf] = true;
    }
    ++m_size;
    if (m_is_list && (m_size > upper_threshhold)) {
      rebuild();
    }
  }

  void remove(const uint64_t suf) {
    if (m_is_list) {
      auto p = std::find(m_list.begin(), m_list.end(), suf);
      assert(p != m_list.end());
      *p = (suffix_t)m_list.back();
      m_list.pop_back();
    } else {
      (*m_bits)[suf] = false;
    }

    --m_size;
    if (!m_is_list && (m_size < lower_threshhold)) {
      rebuild();
    }
  }

  size_t size() {
    return m_size;
  }

  KeyResult<uint64_t> predecessor(int64_t suf) const {
    if (m_is_list) {
      auto p = std::find_if(m_list.begin(), m_list.end(), [suf](const uint64_t x) { return x <= suf; });
      if (p == m_list.end()) {
        return {false, 0};
      }
      suffix_t max_pred = *p;
      for (; p != m_list.end(); ++p) {
        if ((*p) <= suf) {
          max_pred = std::max((suffix_t)*p, max_pred);
        }
      }
      return {true, max_pred};
    } else {
      for (; suf >= 0; --suf) {
        if ((*m_bits)[suf]) {
          return {true, static_cast<uint64_t>(suf)};
        }
      }
      return {false, 0};
    }
  }

  void rebuild() {
    if (m_is_list) {
      m_is_list = false;
      m_size = 0;
      m_bits = new std::bitset<base::MAX_NUM>;
      for (suffix_t key : m_list) {
        set(key);
      }
      // this frees the memory allocated by the vector
      list_t().swap(m_list);
    } else {
      m_is_list = true;
      m_size = 0;
      for (size_t i = 0; i < base::MAX_NUM; ++i) {
        if ((*m_bits)[i]) {
          set(i);
        }
      }
      delete m_bits;
      m_bits = nullptr;
    }
  }
};  // namespace dynamic

}  // namespace dynamic
}  // namespace pred
}  // namespace tdc
