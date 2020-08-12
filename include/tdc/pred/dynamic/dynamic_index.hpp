#pragma once

#include <algorithm>
#include <bitset>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <tdc/pred/dynamic/buckets/buckets.hpp>
#include <tdc/pred/result.hpp>
#include <tdc/util/assert.hpp>
#include <tdc/vec/int_vector.hpp>
#include <vector>

namespace tdc {
namespace pred {
namespace dynamic {

// TODO: use int_vector so that sampling parameter works
/// \brief Dynamic predecessor search using universe-based sampling.
template <template <uint8_t> class t_bucket, uint8_t t_sampling>
class DynIndex {
 private:
  // The bottom part of the data structure is either
  // a bit vector, std::array, or a hybrid.
  static constexpr size_t b_wordl = t_sampling;
  static constexpr size_t b_max = (1ULL << b_wordl) - 1;

  using bucket = t_bucket<b_wordl>;
  uint64_t m_size = 0;                                    // number of keys stored
  uint64_t m_min = std::numeric_limits<uint64_t>::max();  // stores the minimal key
  uint64_t m_max = std::numeric_limits<uint64_t>::min();  // stores the maximal key
  std::vector<bucket *> m_top;                            // top data structure
  bucket *m_first_b = nullptr;                            // pointer to the first bucket

  // return the x_wordl more significant bits of i
  inline uint64_t prefix(uint64_t i) const { return i >> b_wordl; }
  // return the b_wordl less significant bits
  inline uint64_t suffix(uint64_t i) const { return i & b_max; }

 public:
  inline DynIndex() {}

  /// \brief Constructs the index for the given keys.
  /// \param keys a pointer to the keys, that must be in ascending order
  /// \param num the number of keys
  DynIndex(const uint64_t *keys, const size_t num) {
    for (size_t i = 0; i < num; ++i) {
      insert(keys[i]);
    }
  }

  ~DynIndex() {
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
    assert(!predecessor(key).exists || predecessor(key).pos != key);
    const uint64_t key_pre = prefix(key);
    const uint64_t key_suf = suffix(key);
    bucket *new_b;

    // if a key is inserted that needs a greater bucket
    if (key_pre >= m_top.size()) {
      if (tdc_likely(m_size != 0)) {
        new_b = new bucket(key_pre, m_max, nullptr, key_suf);
        m_top.back()->m_next_b = new_b;
        m_top.resize(prefix(key) + 1, m_top.back());
      } else {
        new_b = new bucket(key_pre, 0, nullptr, key_suf);
        m_first_b = new_b;
        m_top.resize(prefix(key) + 1, nullptr);
      }
    } else if (key_pre < prefix(m_min)) {
      // if a key is inserted before the first bucket
      new_b = new bucket(key_pre, 0, m_first_b, key_suf);
      m_first_b = new_b;
    } else {
      // if a key is inserted inbetween
      bucket *key_bucket = m_top[key_pre];
      bucket *next_bucket = key_bucket->m_next_b;
      // if exact bucket exists, add key
      if (key_bucket->m_prefix == key_pre) {
        key_bucket->set(key_suf);
        m_min = std::min(key, m_min);
        m_max = std::max(key, m_max);
        ++m_size;
        assert(predecessor(key).pos == key);
        return;
      } else {
        // if exact bucket does not exist
        new_b = new bucket(key_pre, next_bucket->m_prev_pred, next_bucket, key_suf);
        // pointer in next_bucket
        key_bucket->m_next_b = new_b;
        assert(key_bucket->m_prefix < new_b->m_prefix);
        assert(new_b->m_prefix < next_bucket->m_prefix);
      }
    }

    ++m_size;
    m_min = std::min(key, m_min);
    m_max = std::max(key, m_max);
    // update xfast
    m_top[key_pre] = new_b;
    if (key_pre + 1 < m_top.size()) {
      bucket *next_bucket = m_top[key_pre + 1];
      if (next_bucket == nullptr || next_bucket->m_prefix < key_pre) {
        for (size_t j = key_pre + 1; j < m_top.size() && next_bucket == m_top[j]; ++j) {
          m_top[j] = new_b;
        }
      }
    }
    assert(predecessor(key).pos == key);
  }

  void remove(uint64_t key) {
    assert(predecessor(key).pos == key);
    --m_size;
    const uint64_t key_pre = prefix(key);
    const uint64_t key_suf = suffix(key);
    bucket *const key_bucket = m_top[key_pre];
    bucket *const prev_bucket = m_top[prefix(key_bucket->m_prev_pred)];
    bucket *const next_bucket = key_bucket->m_next_b;

    // if the last key of a bucket was deleted
    key_bucket->remove(suffix(key));
    if (key_bucket->size() == 0) {
      // 3 options: bucket is between 2 buckets; first bucket; last bucket
      if (key_bucket != m_first_b && next_bucket != nullptr) {
        prev_bucket->m_next_b = next_bucket;
        next_bucket->m_prev_pred = key_bucket->m_prev_pred;
        for (uint64_t i = key_pre; i < next_bucket->m_prefix; ++i) {
          m_top[i] = prev_bucket;
        }
      } else if (m_first_b == key_bucket) {
        // if there are buckets left
        if (next_bucket != nullptr) {
          m_first_b = next_bucket;
          m_first_b->m_prev_pred = 0;
          for (uint64_t i = key_pre; i < m_first_b->m_prefix; ++i) {
            m_top[i] = nullptr;
          }
          assert(key == m_min);
          m_min = m_first_b->get_min();
          // if it is the last bucket
        } else {
          m_first_b = nullptr;
          m_min = std::numeric_limits<uint64_t>::max();
          m_max = std::numeric_limits<uint64_t>::min();
          std::vector<bucket *>().swap(m_top);
        }
      } else if (key_bucket == m_top.back()) {
        m_top.resize(prev_bucket->m_prefix + 1);
        m_top.shrink_to_fit();
        m_top.back()->m_next_b = nullptr;
        assert(key == m_max);
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
    if (tdc_unlikely(m_size == 0))
      return Result{false, 1};
    if (tdc_unlikely(x < m_min))
      return Result{false, 0};
    if (tdc_unlikely(x >= m_max))
      return Result{true, m_max};
    return {true, m_top[prefix(x)]->predecessor(x)};
  }
};
}  // namespace dynamic
}  // namespace pred
}  // namespace tdc
