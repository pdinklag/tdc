#pragma once

#include <algorithm>
#include <bitset>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <tdc/pred/result.hpp>
#include <tdc/util/assert.hpp>
#include <tdc/vec/int_vector.hpp>
#include <vector>

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

// TODO: implement delete
// This is a bucket that holds a bit vector.
template <uint8_t b_wordl>
struct bucket_bv {
  uint32_t m_size = 0;
  uint64_t m_prefix;
  uint64_t m_prev_pred = 0;
  bucket_bv *m_next_b = nullptr;  // next bucket
  std::bitset<1ULL << b_wordl> m_bits;

  bucket_bv(uint64_t prefix, uint64_t prev_pred, bucket_bv *next_b)
      : m_prefix(prefix), m_prev_pred(prev_pred), m_next_b(next_b) {}

  uint64_t get_min() {
    for (uint64_t i = 0; i < (1 << b_wordl); ++i) {
      if (m_bits[i]) {
        return (m_prefix << b_wordl) + i;
      }
    }
    return 0;
  }
  void set(const uint64_t i) {
    ++m_size;
    m_bits[i] = true;
  }

  // deletes a key and updates the bottom level of the data structure
  // it can not update the top level
  bool del(uint64_t suf) {
    --m_size;
    m_bits[suf] = false;

    // here we update next_b->prev_pred
    if (tdc_likely(m_next_b != nullptr)) {
      if (m_next_b->m_prev_pred == (m_prefix << b_wordl) + suf) {
        if (m_size != 0) {
          while (!m_bits[suf--]) {
          };
          m_next_b->m_prev_pred = (m_prefix << b_wordl) + suf;
        } else {
          m_next_b->m_prev_pred = m_prev_pred;
        }
      }
    }

    return (m_size == 0);
  }

  uint64_t pred(int64_t i) const {
    i = (i >> b_wordl) != m_prefix ? (1ULL << b_wordl) - 1 : i & ((1ULL << b_wordl) - 1);
    for (; i >= 0; --i) {
      if (m_bits[i]) {
        return (m_prefix << b_wordl) + i;
      }
    }
    return m_prev_pred;
  }
};

// TODO: implement delete
// This is a bucket that holds an std::vector.
template <uint8_t b_wordl>
struct bucket_list {
  uint32_t m_size = 0;
  uint64_t m_prefix;
  uint64_t m_prev_pred = 0;
  bucket_list *m_next_b = nullptr;
  std::vector<uint16_t> m_list;

  bucket_list(uint64_t prefix, uint64_t prev_pred, bucket_list *next_b)
      : m_prefix(prefix), m_prev_pred(prev_pred), m_next_b(next_b) {}

  uint64_t get_min() { return (m_prefix << b_wordl) + *std::min_element(std::begin(m_list), std::end(m_list)); }

  void set(uint64_t i) {
    m_list.push_back(i);
    ++m_size;
  }

  bool del(uint64_t i) { return true; }

  uint64_t pred(int64_t i) const {
    i = (i >> b_wordl) != m_prefix ? (1ULL << b_wordl) - 1 : i & ((1ULL << b_wordl) - 1);
    size_t j = 0;
    uint64_t p;
    bool found = false;
    for (; j < m_list.size(); ++j) {
      if (m_list[j] <= i) {
        p = m_list[j];
        found = true;
        break;
      }
    }
    if (!found) {
      return m_prev_pred;
    }
    for (; j < m_list.size(); ++j) {
      if (m_list[j] > p && m_list[j] <= i) {
        p = m_list[j];
      }
    }
    return (m_prefix << b_wordl) + p;
  }
};

static constexpr size_t upper_threshhold = 1ULL << 8;
static constexpr size_t lower_threshhold = 1ULL << 7;
// TODO: implement delete
// This is a bucket that either holds an std::vector or a bit_vector depending
// on fill rate
template <uint8_t b_wordl>
struct bucket_hybrid {
  uint32_t m_size = 0;
  uint64_t m_prefix;
  uint64_t m_prev_pred = 0;
  bucket_hybrid *m_next_b = nullptr;

  bool m_is_list = true;
  std::vector<uint16_t> m_list;
  std::vector<bool> m_bits;

  bucket_hybrid(uint64_t prefix, uint64_t prev_pred, bucket_hybrid *next_b)
      : m_prefix(prefix), m_prev_pred(prev_pred), m_next_b(next_b) {}

  uint64_t get_min() {
    if (m_is_list) {
      return (m_prefix << b_wordl) + *std::min_element(std::begin(m_list), std::end(m_list));
    } else {
      for (uint64_t i = 0; i < (1 << b_wordl); ++i) {
        if (m_bits[i]) {
          return (m_prefix << b_wordl) + i;
        }
      }
      return 0;
    }
  }

  void set(const uint64_t i) {
    if (m_is_list) {
      m_list.push_back(i);
      ++m_size;
    } else {
      m_bits[i] = true;
      ++m_size;
    }
    if (m_is_list && (m_size > upper_threshhold)) {
      rebuild();
    }
  }

  uint64_t pred(int64_t i) const {
    i = (i >> b_wordl) != m_prefix ? (1ULL << b_wordl) - 1 : i & ((1ULL << b_wordl) - 1);
    if (m_is_list) {
      size_t j = 0;
      uint64_t p;
      bool found = false;
      for (; j < m_list.size(); ++j) {
        if (m_list[j] <= i) {
          p = m_list[j];
          found = true;
          break;
        }
      }
      if (!found) {
        return m_prev_pred;
      }
      for (; j < m_list.size(); ++j) {
        if (m_list[j] > p && m_list[j] <= i) {
          p = m_list[j];
        }
      }
      return (m_prefix << b_wordl) + p;
    } else {
      i = (i >> b_wordl) != m_prefix ? (1ULL << b_wordl) - 1 : i & ((1ULL << b_wordl) - 1);
      for (; i >= 0; --i) {
        if (m_bits[i]) {
          return (m_prefix << b_wordl) + i;
        }
      }
      return m_prev_pred;
    }
  }

  void rebuild() {
    if (m_is_list) {
      m_is_list = false;
      m_size = 0;
      m_bits = std::vector<bool>(1ULL << b_wordl);
      // bits = new std::bitset<1ULL << b_wordl>;
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

// TODO: use int_vector so that sampling parameter works
/// \brief Dynamic predecessor search using universe-based sampling.
template <template <uint8_t> class t_list, uint8_t t_sampling>
class DynIndex {
 private:
  // This data structure needs RAM depending on the greatest key.
  // If you store keys greater than 2^30 you will need a lot of RAM.
  // Here we set greatest key to have 40 bit. wordl = x_wordl + b_wordl.
  static constexpr size_t wordl = 48;
  static constexpr size_t word_size = (1ULL << wordl);
  // The top part of the data structure stores pointers to the bottom buckets.
  static constexpr size_t x_wordl = 40 - t_sampling;
  static constexpr size_t x_size = (1ULL << x_wordl);
  // The bottom part of the data structure is either
  // a bit vector, std::array, or a hybrid.
  static constexpr size_t b_wordl = t_sampling;
  static constexpr size_t b_size = (1ULL << b_wordl);

  using bucket = t_list<b_wordl>;
  uint64_t m_size = 0;                                    // number of keys stored
  uint64_t m_min = std::numeric_limits<uint64_t>::max();  // stores the minimal key
  uint64_t m_max = std::numeric_limits<uint64_t>::min();  // stores the maximal key
  std::vector<bucket *> m_xf;                             // top data structure
  bucket *m_first_b = nullptr;                            // pointer to the first bucket

  // return the x_wordl more significant bits of i
  inline uint64_t prefix(uint64_t i) const { return i >> b_wordl; }
  // return the b_wordl less significant bits
  inline uint64_t suffix(uint64_t i) const { return i & (b_size - 1); }

 public:
  inline DynIndex() { m_xf.resize(0); }

  /// \brief Constructs the index for the given keys.
  /// \param keys a pointer to the keys, that must be in ascending order
  /// \param num the number of keys
  DynIndex(const uint64_t *keys, const size_t num) {
    assert_sorted_ascending(keys, num);
    for (size_t i = 0; i < num; ++i) {
      insert(keys[i]);
    }
  }

  DynIndex(const DynIndex &other) = default;
  DynIndex(DynIndex &&other) = default;
  DynIndex &operator=(const DynIndex &other) = default;
  DynIndex &operator=(DynIndex &&other) = default;
  ~DynIndex() {
    // for (bucket* b : m_xf) {
    //   delete b;
    // }

    bucket *cur = m_first_b;
    bucket *next;
    while (cur != nullptr) {
      next = cur->m_next_b;
      delete cur;
      cur = next;
    }
  }
  
  inline size_t size() const {
    return m_size;
  }

  void insert(const uint64_t key) {
    const uint64_t key_pre = prefix(key);
    const uint64_t key_suf = suffix(key);
    bucket *new_b;

    // if a key is inserted that needs a greater bucket
    if (key_pre >= m_xf.size()) {
      if (tdc_likely(m_size != 0)) {
        new_b = new bucket(key_pre, m_max, nullptr);
        new_b->set(key_suf);
        m_xf.back()->m_next_b = new_b;
        m_xf.resize(prefix(key) + 1, m_xf.back());
      } else {
        new_b = new bucket(key_pre, 0, nullptr);
        new_b->set(key_suf);
        m_first_b = new_b;
        m_xf.resize(prefix(key) + 1, nullptr);
      }
    } else if (key_pre < prefix(m_min)) {
      // if a key is inserted before the first bucket
      new_b = new bucket(key_pre, 0, m_first_b);
      new_b->set(key_suf);
      m_first_b->m_prev_pred = key;
      m_first_b = new_b;
    } else {
      // if a key is inserted inbetween
      bucket *key_bucket = m_xf[key_pre];
      // if exact bucket exists, add key
      if (key_bucket->m_prefix == key_pre) {
        key_bucket->set(key_suf);
        bucket *next_bucket = key_bucket->m_next_b;
        if (next_bucket != nullptr) {
          next_bucket->m_prev_pred = std::max(key, next_bucket->m_prev_pred);
        }
        m_min = std::min(key, m_min);
        m_max = std::max(key, m_max);
        ++m_size;
        return;
      } else {
        // if exact bucket does not exist
        new_b = new bucket(key_pre, key_bucket->m_next_b->m_prev_pred, key_bucket->m_next_b);
        new_b->set(key_suf);
        // pointer in next_bucket
        key_bucket->m_next_b = new_b;
        new_b->m_next_b->m_prev_pred = key;
      }
    }

    ++m_size;
    m_min = std::min(key, m_min);
    m_max = std::max(key, m_max);
    // update xfast
    m_xf[key_pre] = new_b;
    if (key_pre + 1 < m_xf.size()) {
      bucket *next_bucket = m_xf[key_pre + 1];
      if (next_bucket == nullptr || next_bucket->m_prefix < key_pre) {
        for (size_t j = key_pre + 1; j < m_xf.size() && next_bucket == m_xf[j]; ++j) {
          m_xf[j] = new_b;
        }
      }
    }
  }

  void del(const uint64_t key) {
    const uint64_t key_pre = prefix(key);
    const uint64_t key_suf = suffix(key);
    const bucket *key_bucket = m_xf[key_pre];
    const bucket *prev_bucket = m_xf[prefix(key_bucket->m_prev_pred)];
    const bucket *next_bucket = key_bucket->m_next_b;

    // if the last key of a bucket was deleted
    if (key_bucket->del(suffix(key))) {
      // 3 options: bucket is between 2 buckets; first bucket, last bucket
      if (key_bucket != m_first_b && next_bucket != nullptr) {
        prev_bucket->m_next_b = next_bucket;
        for (uint64_t i = key_pre; i < next_bucket->m_prev_pred; ++i) {
          m_xf[i] = prev_bucket;
        }
      } else if (m_first_b == key_bucket) {
        // if there are buckets left
        if (next_bucket != nullptr) {
          m_first_b = next_bucket;
          next_bucket->m_prev_pred = 0;
          for (uint64_t i = key_pre; i < next_bucket->m_prev_pred; ++i) {
            m_xf[i] = nullptr;
          }
          if (key == m_min) {
            m_min = m_first_b->get_min();
          }
          // if it is the last bucket
        } else {
          m_first_b = nullptr;
          m_min = std::numeric_limits<uint64_t>::max();
          m_max = std::numeric_limits<uint64_t>::min();
          m_xf.resize(0);
          m_xf.shrink_to_fit();
        }
      } else if (key_bucket == m_xf.back()) {
        m_xf.resize(prev_bucket->m_prev_pred + 1);
        m_xf.shrink_to_fit();
        m_xf.back()->next_b = nullptr;
        m_max = key_bucket->m_prev_pred;
      }
      // delete the empty bucket
      delete key_bucket;
    }
  }
  /// \brief Finds the rank of the predecessor of the specified key.
  /// \param keys the keys that the compressed trie was constructed for
  /// \param num the number of keys
  /// \param x the key in question
  Result predecessor(const uint64_t x) const {
    if (tdc_unlikely(m_size == 0)) return Result{false, 1};
    if (tdc_unlikely(x < m_min)) return Result{false, 0};
    if (tdc_unlikely(x >= m_max)) return Result{true, m_max};
    return {true, m_xf[prefix(x)]->pred(x)};
  }
};
}  // namespace dynamic
}  // namespace pred
}  // namespace tdc
