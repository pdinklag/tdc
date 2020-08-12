#pragma once

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

  bucket_bv(uint64_t prefix, uint64_t prev_pred, bucket_bv *next_b, uint16_t suf)
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

static constexpr size_t upper_threshhold = 1ULL << 9;
static constexpr size_t lower_threshhold = 1ULL << 8;
// This is a bucket that either holds an std::vector or a bit_vector depending
// on fill rate
template <uint8_t b_wordl>
struct bucket_hybrid {
  uint16_t m_size = 0;
  const uint64_t m_prefix;
  uint64_t m_prev_pred = 0;
  bucket_hybrid *m_next_b = nullptr;

  bool m_is_list = true;
  std::vector<uint16_t> m_list;
  std::vector<bool> m_bits;

  bucket_hybrid(uint64_t prefix, uint64_t prev_pred, bucket_hybrid *next_b, uint16_t suf)
      : m_prefix(prefix), m_prev_pred(prev_pred), m_next_b(next_b) {
    set(suf);
  }

  uint64_t get_min() {
    assert(m_size > 0);
    if (m_is_list) {
      return (m_prefix << b_wordl) + *std::min_element(std::begin(m_list), std::end(m_list));
    } else {
      size_t i = 0;
      while (!m_bits[i]) {
        ++i;
      }
      return (m_prefix << b_wordl) + i;
    }
  }

  void set(const uint64_t suf) {
    if (m_is_list) {
      m_list.push_back(suf);
    } else {
      m_bits[suf] = true;
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
      m_bits[suf] = false;
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
        if (m_bits[key]) {
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
      m_bits = std::vector<bool>(1ULL << b_wordl);
      for (uint16_t key : m_list) {
        set(key);
      }
      // this frees the memory allocated by the vector
      std::vector<uint16_t>().swap(m_list);
    } else {
      m_is_list = true;
      m_size = 0;
      for (size_t i = 0; i < (1ULL << b_wordl); ++i) {
        if (m_bits[i]) {
          set(i);
        }
      }
      std::vector<bool>().swap(m_bits);
    }
  }
};

// This is a bucket that holds an std::vector.
template <template<uint8_t> class t_bucket, uint8_t b_wordl, size_t level>
struct map_bucket {
 private:
  static constexpr uint64_t b_max = (1ULL << b_wordl) - 1;
  static constexpr uint64_t m_max = (1ULL << (b_wordl * (level + 1))) - 1;

  size_t m_size = 0;
  std::vector<uint16_t> m_list;

  std::unordered_map<uint16_t, map_bucket<t_bucket, b_wordl, level - 1>> m_map;

  // get the level'th block of b_wordl bits, level=0 are the last b_wordl bits
  inline uint64_t block(uint64_t i) const { return (i >> (b_wordl * level)) & b_max; }

 public:
  map_bucket() {}

  void insert(uint64_t key) {
    uint64_t b = block(key);
    assert(b < (1ULL << b_wordl));

    if (m_map.find(b) == m_map.end()) {
      m_list.push_back(b);
      m_map.insert(std::make_pair(b, map_bucket<t_bucket, b_wordl, level - 1>()));
      ++m_size;
    }
    m_map.at(b).insert(key);
  }

  void remove(uint64_t key) {
    // uint16_t b = block(key);
    // assert(b < (1ULL << b_wordl));

    // if constexpr (level == 0) {
    //   auto p = std::find(m_list.begin(), m_list.end(), suf);
    //   assert(p != m_list.end());
    //   *p = m_list.back();
    //   m_list.pop_back();
    // }

    // if constexpr (level != 0) {
    //   auto p = m_map.find(b);
    //   assert(p != nullptr);
    //   p.remove(key)
    // }
  }

  size_t size() const {
    return m_size;
  }

  Result pred_in_block(uint64_t b) const {
    auto p = std::find_if(m_list.begin(), m_list.end(), [b](const uint64_t x) { return x <= b; });
    if (p == m_list.end()) {
      return Result{false, 0};
    }
    uint16_t j = (*p);
    for (; p != m_list.end(); ++p) {
      if ((*p) <= b) {
        j = std::max(j, *p);
      }
    }
    return Result{true, j};
  }

  Result true_pred_in_block(uint64_t b) const {
    if (b == 0) {
      return Result{false, 0};
    }
    return pred_in_block(b - 1);
  }

  Result predecessor(uint64_t key) const {
    uint64_t b = block(key);
    assert(b < (1ULL << b_wordl));

    //Look for exact bucket
    auto eb = m_map.find(b);
    if (eb != m_map.end()) {
      Result r = eb->second.predecessor(key);
      if (r.exists) {
        //std::cout << std::hex << "here: " << b << " "<< (b << (b_wordl*level)) << "\n";
        return Result{true, (b << (b_wordl * level)) + r.pos};
      }
    }
    //Look for previous bucket
    Result pre_b = true_pred_in_block(b);
    if (!pre_b.exists) {
      return {false, 0};
    }
    Result pre_m = m_map.at(pre_b.pos).predecessor(m_max);
    assert(pre_m.exists);
    return Result{true, (pre_b.pos << (b_wordl * level)) + pre_m.pos};
  }

  uint64_t min() const {
    uint64_t m = *std::min_element(std::begin(m_list), std::end(m_list));
    return (m << (b_wordl * level)) + m_map.at(m).min();
  }
};

template <template<uint8_t> class t_bucket, uint8_t b_wordl>
struct map_bucket<t_bucket, b_wordl, 0> {
 private:
  size_t m_size = 0;
  std::vector<uint16_t> m_list;

  static constexpr uint64_t b_max = (1ULL << b_wordl) - 1;
  // get the level'th block of b_wordl bits, level=0 are the last b_wordl bits
  inline uint64_t block(uint64_t i) const { return i & b_max; }

 public:
  map_bucket() {}

  void insert(uint64_t key) {
    uint64_t b = block(key);
    assert(b < (1ULL << b_wordl));
    m_list.push_back(b);
    ++m_size;
  }

  void remove(uint64_t key) {
    uint64_t b = block(key);
    assert(b < (1ULL << b_wordl));
    auto p = std::find(m_list.begin(), m_list.end(), b);
    *p = m_list.back();
    m_list.pop_back();
    --m_size;
  }

  size_t size() const {
    return m_size;
  }

  Result predecessor(uint64_t key) const {
    uint64_t b = block(key);
    assert(b < (1ULL << b_wordl));

    auto p = std::find_if(m_list.begin(), m_list.end(), [b](const uint64_t x) { return x <= b; });
    if (p == m_list.end()) {
      return Result{false, 0};
    }
    uint16_t max_pred = (*p);
    for (; p != m_list.end(); ++p) {
      if ((*p) <= b) {
        max_pred = std::max(max_pred, *p);
      }
    }
    return Result{true, max_pred};
  }

  uint64_t min() const {
    return *std::min_element(std::begin(m_list), std::end(m_list));
  }
};

}  // namespace dynamic
}  // namespace pred
}  // namespace tdc