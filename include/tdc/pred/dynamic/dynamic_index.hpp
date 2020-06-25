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

/// \brief Dynamic predecessor search using universe-based sampling.

class DynIndex {
 private:
  using uni_t = uint32_t;
  static constexpr size_t wordl = sizeof(uni_t) * 16;
  static constexpr size_t b_wordl = 16;
  static constexpr size_t x_wordl = 16;
  static constexpr size_t x_size = (1ULL << x_wordl);

  struct bucket {
    uint64_t prefix;
    bucket* prev_b = nullptr;  // previous bucket
    bucket* next_b = nullptr;  // next bucket
    std::bitset<x_size> bits;

    void set(uint64_t i) {
      bits[i] = true;
    }

    bool check(unsigned char i) const {
      return bits[i];
    }

    uint64_t pred(uint64_t i) const {
      for (; i != 0; --i) {
        if (bits[i]) {
          return (prefix << b_wordl) + i;
        }
      }
      if (bits[0]) {
        return (prefix << b_wordl) + 0;
      } else {
        return next_b->pred(x_size - 1);
      }
    }
  };

  mutable Result m_min = {false, 0};
  mutable Result m_max = {false, std::numeric_limits<uint64_t>::max()};
  mutable std::vector<bucket*> m_xf;
  mutable bucket* m_first_b = nullptr;

  mutable bool m_is_updated = true;
  mutable std::vector<uint64_t> m_keys;

  inline uint64_t prefix(uint64_t i) const { return i >> b_wordl; }
  inline uint64_t suffix(uint64_t i) const {
    return i & ((1ULL << (b_wordl)) - 1);
  }

  inline void build_top(const uint64_t* keys, uint64_t num) const {}

  inline void update() const {
    std::sort(m_keys.begin(), m_keys.end());
    if (prefix(m_keys.back()) >= m_xf.size()) {
      m_xf.resize(prefix(m_keys.back()) + 1, nullptr);
    }
    if (!m_min.exists) {
      m_min = {true, m_keys.front()};
      m_max = {true, m_keys.back()};
    } else {
      if (m_keys.front() < m_min.pos) {
        m_min = {true, m_keys.front()};
      }
      if (m_keys.back() > m_max.pos) {
        m_max = {true, m_keys.back()};
      }
    }

    for (uint64_t i = 0; i < m_keys.size(); ++i) {
      // update bucket
      uint64_t key = m_keys[i];
      uint64_t key_pre = prefix(key);
      uint64_t key_suf = suffix(key);
      bucket* key_b_old = m_xf[key_pre];
      bucket* new_b;
      // if previous bucket exists
      if (key_b_old != nullptr) {
        // if exact bucket exists, add key
        if (key_b_old->prefix == key_pre) {
          key_b_old->set(key_suf);
          continue;
        } else {
          // if exact bucket doesnt exist, create bucket
          new_b = new bucket;
          new_b->prefix = key_pre;
          new_b->set(key_suf);
          // pointer in new_bucket
          new_b->prev_b = key_b_old;
          new_b->next_b = key_b_old->next_b;
          // pointers in prev and next bucket
          new_b->prev_b->next_b = new_b;
          if (new_b->next_b != nullptr) {
            new_b->next_b->prev_b = new_b;
          }
        }
      } else {
        // if no prev bucket exists, create bucket
        new_b = new bucket;
        new_b->prefix = key_pre;
        new_b->set(key_suf);
        // pointer in new bucket
        new_b->next_b = m_first_b;
        m_first_b = new_b;
        // pointer in next bucket
        if (new_b->next_b != nullptr) {
          new_b->next_b->prev_b = new_b;
        }
      }
      // now update xfast
      uint64_t j = key_pre;
      // if there is a next key
      if (i + 1 < m_keys.size()) {
        while (m_xf[j] == key_b_old && j <= prefix(m_keys[i + 1])) {
          m_xf[j] = new_b;
          ++j;
        }
      } else {
        // if there is no next key
        while (m_xf[j] == key_b_old && j < x_size) {
          m_xf[j] = new_b;
          ++j;
        }
      }
    }
    // m_keys.clear(); //TODO: ADD THAT AGAIN
    m_is_updated = true;
  }

 public:
  inline DynIndex() {
    // m_xf.resize(x_size, nullptr);
    m_xf.resize(0);
    m_keys.resize(0);
  }

  /// \brief Constructs the index for the given keys.
  /// \param keys a pointer to the keys, that must be in ascending order
  /// \param num the number of keys
  /// \param lo_bits the number of low key bits, defining the maximum size of
  /// a search interval; lower means faster queries, but more memory usage
  DynIndex(const uint64_t* keys, const size_t num) {
    assert_sorted_ascending(keys, num);
    // build an index for high bits
    build_top(keys, num);
  }

  DynIndex(const DynIndex& other) = default;
  DynIndex(DynIndex&& other) = default;
  DynIndex& operator=(const DynIndex& other) = default;
  DynIndex& operator=(DynIndex&& other) = default;
  ~DynIndex() {
    for(bucket* b : m_xf) {
      delete b;
    }
  }

  void insert(const uint64_t x) {
    m_is_updated = false;
    m_keys.push_back(x);
  }

  /// \brief Finds the rank of the predecessor of the specified key.
  /// \param keys the keys that the compressed trie was constructed for
  /// \param num the number of keys
  /// \param x the key in question
  Result predecessor(const uint64_t x) const {
    if (!m_is_updated) {
      update();
    }
    if (tdc_unlikely(!m_min.exists)) return Result{false, 1};
    if (tdc_unlikely(x < m_min.pos)) return Result{false, 0};
    // if (tdc_unlikely(x >= m_max)) return Result{true, m_keys.size() - 1};
    return {true, m_xf[prefix(x)]->pred(suffix(x))};
  }
};  // namespace dynamic

}  // namespace dynamic
}  // namespace pred
}  // namespace tdc