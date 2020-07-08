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

class DynIndexList {
 private:
  using uni_t = uint32_t;
  static constexpr size_t wordl = sizeof(uni_t) * 16;
  static constexpr size_t b_wordl = 16;
  static constexpr size_t x_wordl = 16;
  static constexpr size_t x_size = (1ULL << x_wordl);

  struct bucket_bv {
    uint64_t prefix;
    uint64_t prev_pred = 0;

    bucket_bv* next_b = nullptr;  // next bucket
    std::bitset<x_size> bits;

    void set(uint64_t i) { bits[i] = true; }

    //bool check(unsigned char i) const { return bits[i]; }

    uint64_t pred(int64_t i) const {
      for (; i >= 0; --i) {
        if (bits[i]) {
          return (prefix << b_wordl) + i;
        }
      }
      return prev_pred;
    }
  };

  struct bucket_list {
    uint64_t prefix;
    uint64_t prev_pred = 0;

    bucket_list* next_b = nullptr;
    std::vector<uint16_t> list;

    void set(uint64_t i) { list.push_back(i); }
    uint64_t pred(int64_t i) const {
      size_t j = 0;
      uint64_t p;
      bool found = false;
      while (j < list.size()) {
        if (list[j] <= i) {
          p = list[j];
          found = true;
          break;
        }
        ++j;
      }
      if (!found) {
        return prev_pred;
      }
      while (j < list.size()) {
        if (list[j] > p && list[j] <= i) {
          p = list[j];
        }
        ++j;
      }
      return (prefix << b_wordl) + p;
    }
  };

  using bucket = bucket_list;
  mutable Result m_min = {false, 0};
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
    m_is_updated = true;
    std::sort(m_keys.begin(), m_keys.end());
    if (prefix(m_keys.back()) >= m_xf.size()) {
      m_xf.resize(prefix(m_keys.back()) + 1, nullptr);
    }
    if (!m_min.exists) {
      m_min = {true, m_keys.front()};
    } else {
      if (m_keys.front() < m_min.pos) {
        m_min = {true, m_keys.front()};
      }
    }

    uint64_t last_updated_x = prefix(m_keys[0]);
    for (uint64_t i = 0; i < m_keys.size(); ++i) {
      // update bucket
      uint64_t key = m_keys[i];
      uint64_t key_pre = prefix(key);
      uint64_t key_suf = suffix(key);
      bucket* old_prev_b = m_xf[key_pre];
      bucket* new_b;
      // if previous bucket exists
      if (old_prev_b != nullptr) {
        // if exact bucket exists, add key
        if (old_prev_b->prefix == key_pre) {
          old_prev_b->set(key_suf);
          bucket* next_bucket = old_prev_b->next_b;
          if (next_bucket != nullptr) {
            next_bucket->prev_pred =
                std::max(m_keys[i], next_bucket->prev_pred);
          }
          continue;
        } else {
          // if exact bucket doesnt exist, create bucket
          new_b = new bucket;
          new_b->prefix = key_pre;
          new_b->next_b = old_prev_b->next_b;
          new_b->set(key_suf);
          // pointers in prev bucket
          old_prev_b->next_b = new_b;
          new_b->prev_pred = m_xf[key_pre - 1]->pred(x_size - 1);
          // and update xfast
        }
      } else {
        // if no prev bucket exists, create bucket
        if (m_first_b != nullptr) {
          m_first_b->prev_pred = m_keys[i];
        }
        new_b = new bucket;
        new_b->prefix = key_pre;
        new_b->prev_pred = 0;
        new_b->next_b = m_first_b;
        m_first_b = new_b;
        new_b->set(key_suf);
      }

      // update xfast
      m_xf[key_pre] = new_b;

      size_t check_to = m_xf.size();
      for (size_t j = key_pre + 1; j < check_to; ++j) {
        if (m_xf[j] == nullptr || m_xf[j]->prefix < new_b->prefix) {
          m_xf[j] = new_b;
          ++j;
        } else {
          break;
        }
      }
    }
    m_keys.clear();
  }

 public:
  inline DynIndexList() {
    m_xf.resize(0);
    m_keys.resize(0);
  }

  /// \brief Constructs the index for the given keys.
  /// \param keys a pointer to the keys, that must be in ascending order
  /// \param num the number of keys
  /// \param lo_bits the number of low key bits, defining the maximum size of
  /// a search interval; lower means faster queries, but more memory usage
  DynIndexList(const uint64_t* keys, const size_t num) {
    assert_sorted_ascending(keys, num);
    // build an index for high bits
    build_top(keys, num);
  }

  DynIndexList(const DynIndexList& other) = default;
  DynIndexList(DynIndexList&& other) = default;
  DynIndexList& operator=(const DynIndexList& other) = default;
  DynIndexList& operator=(DynIndexList&& other) = default;
  ~DynIndexList() {
    for (bucket* b : m_xf) {
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
