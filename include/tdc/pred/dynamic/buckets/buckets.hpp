#pragma once

#include <cassert>
#include <limits>
#include <memory>
#include <type_traits>

#include <tdc/pred/binary_search.hpp>
#include <tdc/math/bit_mask.hpp>
#include <tdc/util/hybrid_ptr.hpp>
#include <tdc/vec/fixed_width_int_vector.hpp>

namespace tdc {
namespace pred {
namespace dynamic {

template<uint8_t b_wordl>
struct bucket_base {
    static_assert(b_wordl <= 32, "bucket sizes beyond 2^32 not supported");
    
    static constexpr uint8_t suffix_bits = b_wordl;
    
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
};

// This is a bucket that holds a bit vector.
template <typename key_t, uint8_t b_wordl>
struct bucket_bv : bucket_base<b_wordl> {
  using base = bucket_base<b_wordl>;
  using suffix_t = typename base::suffix_t;
    
  const uint64_t m_prefix;
  uint64_t m_prev_pred = 0;
  bucket_bv *m_next_b = nullptr;  // next bucket
  std::bitset<base::MAX_NUM> m_bits;
  suffix_t m_size = 0;

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
template <typename key_t, uint8_t b_wordl, size_t upper_threshold = 1ULL << 9>
struct bucket_hybrid : bucket_base<b_wordl> {
  using base = bucket_base<b_wordl>;
  using suffix_t = typename base::suffix_t;
  using list_t = typename base::list_t;
  using bv_t = std::bitset<base::MAX_NUM>;
  
  static constexpr size_t lower_threshold = upper_threshold / 2;
  
  const uint64_t m_prefix;
  uint64_t m_prev_pred = 0;
  bucket_hybrid* m_next_b;
  hybrid_ptr<list_t, bv_t> m_ptr;

  suffix_t m_size = 0;

  bucket_hybrid(uint64_t prefix, uint64_t prev_pred, bucket_hybrid *next_b, suffix_t suf)
      : m_prefix(prefix), m_prev_pred(prev_pred), m_next_b(next_b) {

    m_ptr = std::make_unique<list_t>(); // start as a list
    set(suf);
  }

  uint64_t get_min() {
    assert(m_size > 0);
    if (m_ptr.is_first()) {
      auto& list = *m_ptr.as_first();
      return (m_prefix << b_wordl) + *std::min_element(std::begin(list), std::end(list));
    } else {
      auto& bv = *m_ptr.as_second();
      size_t i = 0;
      while (!bv[i]) {
        ++i;
      }
      return (m_prefix << b_wordl) + i;
    }
  }

  void set(const uint64_t suf) {
    if (m_ptr.is_first()) {
      m_ptr.as_first()->push_back(suf);
    } else {
      (*m_ptr.as_second())[suf] = true;
    }

    ++m_size;
    if (tdc_likely(m_next_b != nullptr)) {
      m_next_b->m_prev_pred = std::max((m_prefix << b_wordl) + suf, m_next_b->m_prev_pred);
    }
    if (m_ptr.is_first() && (m_size >= upper_threshold)) {
      rebuild();
    }
  }

  void remove(const uint64_t suf) {
    if (m_ptr.is_first()) {
      auto& list = *m_ptr.as_first();
      auto p = std::find(list.begin(), list.end(), suf);
      assert(p != list.end());
      *p = (suffix_t)list.back();
      list.pop_back();
    } else {
      (*m_ptr.as_second())[suf] = false;
    }

    --m_size;
    // here we update next_b->prev_pred
    if (tdc_likely(m_next_b != nullptr)) {
      if (m_next_b->m_prev_pred == (m_prefix << b_wordl) + suf) {
        m_next_b->m_prev_pred = predecessor((m_prefix << b_wordl) + suf);
      }
    }
    if (!m_ptr.is_first() && (m_size < lower_threshold)) {
      rebuild();
    }
  }

  size_t size() {
    return m_size;
  }

  uint64_t predecessor(int64_t key) const {
    key = base::prefix(key) != m_prefix ? base::SUFFIX_MAX : base::suffix(key);
    if (m_ptr.is_first()) {
      auto& list = *m_ptr.as_first();
      auto p = std::find_if(list.begin(), list.end(), [key](const uint64_t x) { return x <= key; });
      if (p == list.end()) {
        return m_prev_pred;
      }
      suffix_t max_pred = *p;
      for (; p != list.end(); ++p) {
        if ((*p) <= key) {
          max_pred = std::max((suffix_t)*p, max_pred);
        }
      }
      return (m_prefix << b_wordl) + max_pred;
    } else {
      auto& bv = *m_ptr.as_second();
      for (; key >= 0; --key) {
        if (bv[key]) {
          return (m_prefix << b_wordl) + key;
        }
      }
      return m_prev_pred;
    }
  }

  void
  rebuild() {
    if (m_ptr.is_first()) {
      m_size = 0;
      auto plist = m_ptr.release_as_first();
      m_ptr = std::make_unique<bv_t>();
      for (suffix_t key : *plist) {
        set(key);
      }
    } else {
      auto pbv = m_ptr.release_as_second();
      m_ptr = std::make_unique<list_t>((size_t)m_size);
      m_size = 0;
      for (size_t i = 0; i < base::MAX_NUM; ++i) {
        if ((*pbv)[i]) {
          set(i);
        }
      }
    }
  }
} __attribute__((__packed__));

template <typename key_t, uint8_t b_wordl>
struct map_bucket_bv : bucket_base<b_wordl> {
  using base = bucket_base<b_wordl>;
  using suffix_t = typename base::suffix_t;
    
  std::bitset<base::MAX_NUM>* m_bits;
  suffix_t m_size = 0;

  map_bucket_bv() {
    m_bits = new std::bitset<base::MAX_NUM>();
  }

  map_bucket_bv(uint64_t suf) {
    m_bits = new std::bitset<base::MAX_NUM>();
    set(suf);
  }
  
  ~map_bucket_bv() {
    delete m_bits;
  }
  
  map_bucket_bv(const map_bucket_bv&) = delete;
  map_bucket_bv(map_bucket_bv&&) = delete;
  map_bucket_bv& operator=(const map_bucket_bv&) = delete;
  map_bucket_bv& operator=(map_bucket_bv&&) = delete;

  uint64_t get_min() {
    assert(m_size > 0);
    size_t suf = 0;
    while (!(*m_bits)[suf]) {
      ++suf;
    }
    return suf;
  }
  void set(const uint64_t suf) {
    (*m_bits)[suf] = true;
    ++m_size;
  }

  void remove(uint64_t suf) {
    (*m_bits)[suf] = false;
    --m_size;
  }

  size_t size() {
    return m_size;
  }

  KeyResult<uint64_t> predecessor(int64_t suf) const {
    for (; suf >= 0; --suf) {
      if ((*m_bits)[suf]) {
        return {true, static_cast<uint64_t>(suf)};
      }
    }
    return {false, 0};
  }
} __attribute__((__packed__));

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
  
  map_bucket_list(const map_bucket_list&) = delete;
  map_bucket_list(map_bucket_list&&) = delete;
  map_bucket_list& operator=(const map_bucket_list&) = delete;
  map_bucket_list& operator=(map_bucket_list&&) = delete;

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

// This is a bucket that holds a sorted std::vector.
template <typename key_t, uint8_t b_wordl>
struct map_bucket_slist : bucket_base<b_wordl> {
  using base = bucket_base<b_wordl>;
  using suffix_t = typename base::suffix_t;
  using list_t = typename base::list_t;

  list_t m_list;

  map_bucket_slist() {
  }

  map_bucket_slist(uint64_t suf) {
    set(suf);
  }
  
  map_bucket_slist(const map_bucket_slist&) = delete;
  map_bucket_slist(map_bucket_slist&&) = delete;
  map_bucket_slist& operator=(const map_bucket_slist&) = delete;
  map_bucket_slist& operator=(map_bucket_slist&&) = delete;

  uint64_t get_min() {
    assert(m_list.size() > 0);
    return m_list[0];
  }

  void set(uint64_t suf) {
    const auto sz = size();
    if(sz > 0) {
        // find rank
        auto rank = BinarySearch<suffix_t>::predecessor(m_list, sz, suf);
        const size_t insert_pos = rank.exists ? rank.pos + 1 : 0;
        
        // insert
        m_list.push_back((suffix_t)m_list.back()); // duplicate largest
        for(size_t i = sz - 1; i > insert_pos; i--) {
            m_list[i] = (suffix_t)m_list[i-1];
        }
        m_list[insert_pos] = suf;
    } else {
        m_list.push_back(suf);
    }
  }

  void remove(uint64_t suf) {
    auto pred = BinarySearch<suffix_t>::predecessor(m_list, size(), suf);
    assert(pred.exists);
    assert(m_list[pred.pos] == suf);
    
    for(size_t i = pred.pos; i < size() - 1; i++) {
        m_list[i] = (suffix_t)m_list[i + 1];
    }
    m_list.pop_back();
  }

  size_t size() const {
    return m_list.size();
  }

  KeyResult<uint64_t> predecessor(int64_t suf) const {
    auto pred = BinarySearch<suffix_t>::predecessor(m_list, size(), suf);
    if(pred.exists) {
        return {true, m_list[pred.pos]};
    } else {
        return {false, 0};
    }
  }
};

// This is a bucket that either holds an std::vector or a bit_vector depending
// on fill rate
template <typename key_t, uint8_t b_wordl, size_t upper_threshold = 1ULL << 9>
struct map_bucket_hybrid : bucket_base<b_wordl> {
  using base = bucket_base<b_wordl>;
  using suffix_t = typename base::suffix_t;
  using list_t = typename base::list_t;
  using bv_t = std::bitset<base::MAX_NUM>;
    
  static constexpr size_t lower_threshold = upper_threshold / 2;

  hybrid_ptr<list_t, bv_t> m_ptr;
  suffix_t m_size = 0;

  map_bucket_hybrid() {
    m_ptr = std::make_unique<list_t>(); // start as a list
  }

  map_bucket_hybrid(suffix_t suf) {
    m_ptr = std::make_unique<list_t>(); // start as a list
    set(suf);
  }

  map_bucket_hybrid(const map_bucket_hybrid& other) = delete;
  map_bucket_hybrid(map_bucket_hybrid&& other) = delete;
  map_bucket_hybrid& operator=(const map_bucket_hybrid& other) = delete;
  map_bucket_hybrid& operator=(map_bucket_hybrid&& other) = delete;

  uint64_t get_min() {
    assert(m_size > 0);
    if (m_ptr.is_first()) {
      auto& list = *m_ptr.as_first();
      return *std::min_element(std::begin(list), std::end(list));
    } else {
      size_t suf = 0;
      while (!(*m_ptr.as_second())[suf]) {
        ++suf;
      }
      return suf;
    }
  }
  void set(const uint64_t suf) {
    if (m_ptr.is_first()) {
      m_ptr.as_first()->push_back(suf);
    } else {
      (*m_ptr.as_second())[suf] = true;
    }
    ++m_size;
    if (m_ptr.is_first() && (m_size >= upper_threshold)) {
      rebuild();
    }
  }

  void remove(const uint64_t suf) {
    if (m_ptr.is_first()) {
      auto& list = *m_ptr.as_first();
      auto p = std::find(list.begin(), list.end(), suf);
      assert(p != list.end());
      *p = (suffix_t)list.back();
      list.pop_back();
    } else {
      (*m_ptr.as_second())[suf] = false;
    }

    --m_size;
    if (!m_ptr.is_first() && (m_size < lower_threshold)) {
      rebuild();
    }
  }

  size_t size() {
    return m_size;
  }

  KeyResult<uint64_t> predecessor(int64_t suf) const {
    if (m_ptr.is_first()) {
      auto& list = *m_ptr.as_first();
      auto p = std::find_if(list.begin(), list.end(), [suf](const uint64_t x) { return x <= suf; });
      if (p == list.end()) {
        return {false, 0};
      }
      suffix_t max_pred = *p;
      for (; p != list.end(); ++p) {
        if ((*p) <= suf) {
          max_pred = std::max((suffix_t)*p, max_pred);
        }
      }
      return {true, max_pred};
    } else {
      auto& bv = *m_ptr.as_second();
      for (; suf >= 0; --suf) {
        if (bv[suf]) {
          return {true, static_cast<uint64_t>(suf)};
        }
      }
      return {false, 0};
    }
  }

  void rebuild() {
    if (m_ptr.is_first()) {
      m_size = 0;
      auto plist = m_ptr.release_as_first();
      m_ptr = std::make_unique<bv_t>();
      for (suffix_t key : *plist) {
        set(key);
      }
    } else {
      auto pbv = m_ptr.release_as_second();
      m_ptr = std::make_unique<list_t>((size_t)m_size);
      m_size = 0;
      for (size_t i = 0; i < base::MAX_NUM; ++i) {
        if ((*pbv)[i]) {
          set(i);
        }
      }
    }
  }
} __attribute__((__packed__));

}  // namespace dynamic
}  // namespace pred
}  // namespace tdc
