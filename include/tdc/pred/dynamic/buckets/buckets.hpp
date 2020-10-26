#pragma once

#include <assert.h>

namespace tdc {
namespace pred {
namespace dynamic {

// return the x_wordl more significant bits of i
template <uint8_t b_wordl>
inline uint64_t prefix(uint64_t i) {
  return i >> b_wordl;
}
// return the b_wordl less significant bits
template <uint8_t b_wordl>
inline uint64_t suffix(uint64_t i) {
  return i & ((1ULL << b_wordl) - 1);
}

// This is a bucket that holds a bit vector.
template <uint8_t b_wordl>
struct bucket_bv {
  uint16_t m_size = 0;
  const uint64_t m_prefix;
  uint64_t m_prev_pred = 0;
  bucket_bv *m_next_b = nullptr;  // next bucket
  std::bitset<1ULL << b_wordl> m_bits;

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
    key = prefix<b_wordl>(key) != m_prefix ? (1ULL << b_wordl) - 1 : suffix<b_wordl>(key);
    for (; key >= 0; --key) {
      if (m_bits[key]) {
        return (m_prefix << b_wordl) + key;
      }
    }
    return m_prev_pred;
  }
};

// This is a bucket that holds an std::vector.
template <uint8_t b_wordl>
struct bucket_list {
  uint16_t m_size = 0;
  const uint64_t m_prefix;
  uint64_t m_prev_pred = 0;
  bucket_list *m_next_b = nullptr;
  std::vector<uint16_t> m_list;

  bucket_list(uint64_t prefix, uint64_t prev_pred, bucket_list *next_b, uint16_t suf)
      : m_prefix(prefix), m_prev_pred(prev_pred), m_next_b(next_b) {
    set(suf);
  }

  uint64_t get_min() {
    assert(m_size > 0);
    return (m_prefix << b_wordl) + *std::min_element(std::begin(m_list), std::end(m_list));
  }

  void set(uint64_t suf) {
    m_list.push_back(suf);
    ++m_size;
    if (tdc_likely(m_next_b != nullptr)) {
      m_next_b->m_prev_pred = std::max((m_prefix << b_wordl) + suf, m_next_b->m_prev_pred);
    }
  }

  void remove(uint64_t suf) {
    auto p = std::find(m_list.begin(), m_list.end(), suf);
    assert(p != m_list.end());
    *p = m_list.back();
    m_list.pop_back();
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
    key = prefix<b_wordl>(key) != m_prefix ? (1ULL << b_wordl) - 1 : suffix<b_wordl>(key);

    auto p = std::find_if(m_list.begin(), m_list.end(), [key](const uint64_t x) { return x <= key; });
    if (p == m_list.end()) {
      return m_prev_pred;
    }
    uint16_t max_pred = *p;
    for (; p != m_list.end(); ++p) {
      if ((*p) <= key) {
        max_pred = std::max(*p, max_pred);
      }
    }
    return (m_prefix << b_wordl) + max_pred;
  }
};

// This is a bucket that either holds an std::vector or a bit_vector depending
// on fill rate
template <uint8_t b_wordl>
struct bucket_hybrid {
  static constexpr size_t upper_threshhold = 1ULL << 9;
  static constexpr size_t lower_threshhold = 1ULL << 8;
  uint16_t m_size = 0;
  const uint64_t m_prefix;
  uint64_t m_prev_pred = 0;
  bucket_hybrid *m_next_b = nullptr;

  bool m_is_list = true;
  std::vector<uint16_t> m_list;
  std::bitset<1ULL<<b_wordl>* m_bits = nullptr;

  bucket_hybrid(uint64_t prefix, uint64_t prev_pred, bucket_hybrid *next_b, uint16_t suf)
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
      *p = m_list.back();
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
    key = prefix<b_wordl>(key) != m_prefix ? (1ULL << b_wordl) - 1 : suffix<b_wordl>(key);
    if (m_is_list) {
      auto p = std::find_if(m_list.begin(), m_list.end(), [key](const uint64_t x) { return x <= key; });
      if (p == m_list.end()) {
        return m_prev_pred;
      }
      uint16_t max_pred = *p;
      for (; p != m_list.end(); ++p) {
        if ((*p) <= key) {
          max_pred = std::max(*p, max_pred);
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
      m_bits = new std::bitset<1ULL<<b_wordl>; //std::vector<bool>(1ULL << b_wordl);
      for (uint16_t key : m_list) {
        set(key);
      }
      // this frees the memory allocated by the vector
      std::vector<uint16_t>().swap(m_list);
    } else {
      m_is_list = true;
      m_size = 0;
      for (size_t i = 0; i < (1ULL << b_wordl); ++i) {
        if ((*m_bits)[i]) {
          set(i);
        }
      }
      delete m_bits;
      m_bits = nullptr;
    }
  }
};

template <uint8_t b_wordl>
struct map_bucket_bv {
  uint16_t m_size = 0;
  std::bitset<1ULL << b_wordl> m_bits;

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
template <uint8_t b_wordl>
struct map_bucket_list {
  uint16_t m_size = 0;
  std::vector<uint16_t> m_list;

  map_bucket_list() {
  }

  map_bucket_list(uint64_t suf) {
    set(suf);
  }

  uint64_t get_min() {
    assert(m_size > 0);
    return *std::min_element(std::begin(m_list), std::end(m_list));
  }

  void set(uint64_t suf) {
    m_list.push_back(suf);
    ++m_size;
  }

  void remove(uint64_t suf) {
    auto p = std::find(m_list.begin(), m_list.end(), suf);
    assert(p != m_list.end());
    *p = m_list.back();
    m_list.pop_back();
    --m_size;
  }

  size_t size() {
    return m_size;
  }

  KeyResult<uint64_t> predecessor(int64_t suf) const {
    auto p = std::find_if(m_list.begin(), m_list.end(), [suf](const uint64_t x) { return x <= suf; });
    if (p == m_list.end()) {
      return {false, 0};
    }
    uint16_t max_pred = *p;
    for (; p != m_list.end(); ++p) {
      if ((*p) <= suf) {
        max_pred = std::max(*p, max_pred);
      }
    }
    return {true, max_pred};
  }
};

// This is a bucket that either holds an std::vector or a bit_vector depending
// on fill rate
template <uint8_t b_wordl>
struct map_bucket_hybrid {
  static constexpr size_t upper_threshhold = 1ULL << 9;
  static constexpr size_t lower_threshhold = 1ULL << 8;
  uint16_t m_size = 0;
  bool m_is_list = true;
  std::vector<uint16_t> m_list;
  std::bitset<1ULL << b_wordl> *m_bits = nullptr;

  map_bucket_hybrid() {
  }

  map_bucket_hybrid(uint16_t suf) {
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
      *p = m_list.back();
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
      uint16_t max_pred = *p;
      for (; p != m_list.end(); ++p) {
        if ((*p) <= suf) {
          max_pred = std::max(*p, max_pred);
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
      m_bits = new std::bitset<1ULL<<b_wordl>;
      for (uint16_t key : m_list) {
        set(key);
      }
      // this frees the memory allocated by the vector
      std::vector<uint16_t>().swap(m_list);
    } else {
      m_is_list = true;
      m_size = 0;
      for (size_t i = 0; i < (1ULL << b_wordl); ++i) {
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
