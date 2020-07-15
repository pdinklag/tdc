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

template<bool t_list>
class DynIndex {
 private:
  using uni_t = uint32_t;
  static constexpr size_t wordl = sizeof(uni_t) * 16;
  static constexpr size_t b_wordl = 16;
  static constexpr size_t b_size = (1ULL << b_wordl);
  static constexpr size_t x_wordl = 16;
  static constexpr size_t x_size = (1ULL << x_wordl);

  struct bucket_bv {
    uint64_t prefix;
    uint64_t prev_pred = 0;

    bucket_bv* next_b = nullptr;  // next bucket
    std::bitset<b_size> bits;

    void set(uint64_t i) { bits[i] = true; }

    // bool check(unsigned char i) const { return bits[i]; }

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
      for (; j < list.size(); ++j) {
        if (list[j] <= i) {
          p = list[j];
          found = true;
          break;
        }
      }
      if (!found) {
        return prev_pred;
      }
      for (; j < list.size(); ++j) {
        if (list[j] > p && list[j] <= i) {
          p = list[j];
        }
      }
      return (prefix << b_wordl) + p;
    }
  };

  using bucket = typename std::conditional<t_list, bucket_list, bucket_bv>::type;//using bucket = bucket_bv;
  mutable Result m_min = {false, 0};
  mutable Result m_max = {false, 0};
  mutable std::vector<bucket*> m_xf;
  mutable bucket* m_first_b = nullptr;

  mutable std::vector<uint64_t> m_keys;

  inline uint64_t prefix(uint64_t i) const { return i >> b_wordl; }
  inline uint64_t suffix(uint64_t i) const {
    return i & ((1ULL << (b_wordl)) - 1);
  }

 public:
  inline DynIndex() {
    m_xf.resize(1, nullptr);
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
    for(size_t i = 0; i < num; ++i) {
      insert(keys[i]);
    }
  }

  DynIndex(const DynIndex& other) = default;
  DynIndex(DynIndex&& other) = default;
  DynIndex& operator=(const DynIndex& other) = default;
  DynIndex& operator=(DynIndex&& other) = default;
  ~DynIndex() {
    for (bucket* b : m_xf) {
      delete b;
    }
  }

  void insert(const uint64_t key) {
    if (prefix(key) >= m_xf.size()) {
      m_xf.resize(prefix(key) + 1, m_xf.back());
    }
    if (!m_min.exists) {
      m_min = {true, key};
      m_max = {true, key};
    } else {
      if (key < m_min.pos) {
        m_min = {true, key};
      }
      if (key > m_max.pos) {
        m_max = {true, key};
      }
    }
    uint64_t key_pre = prefix(key);
    uint64_t key_suf = suffix(key);
    bucket* key_bucket = m_xf[key_pre];
    bucket* new_b;
    // if previous bucket exists
    if (key_bucket != nullptr) {
      // if exact bucket exists, add key
      if (key_bucket->prefix == key_pre) {
        key_bucket->set(key_suf);
        bucket* next_bucket = key_bucket->next_b;
        if (next_bucket != nullptr) {  // Das tritt genau ein wenn
                                       // key_bucket->prefix < m_xf.size()-1
          next_bucket->prev_pred = std::max(key, next_bucket->prev_pred);
        }
        return;
      } else {  // if exact bucket does not exist
        new_b = new bucket;
        new_b->prefix = key_pre;
        new_b->next_b = key_bucket->next_b;
        new_b->set(key_suf);
        // pointer in next_bucket
        key_bucket->next_b = new_b;
        new_b->prev_pred = m_xf[key_pre - 1]->pred(
            b_size -
            1);  // Das kommt mir spanisch vor, lieber nochmal unterscheiden ob
                 // wir einen bucket ans ende oder in die mitte einfügen
        bucket* next_bucket = new_b->next_b;  // siehe kommentar ^
        if (next_bucket != nullptr) {
          next_bucket->prev_pred = key;
        }
      }
    } else {  // TODO: Das tritt nur ein, falls x < min. Schiebe das also hoch
      // if no prev bucket exists, create bucket
      if (m_first_b != nullptr) {
        m_first_b->prev_pred = key;
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
      // Improvement: save pointer and compare pointer instead of prefix value
      // of pointer
      if (m_xf[j] == nullptr || m_xf[j]->prefix < new_b->prefix) {
        m_xf[j] = new_b;
      } else {
        break;
      }
    }
  }

  /// \brief Finds the rank of the predecessor of the specified key.
  /// \param keys the keys that the compressed trie was constructed for
  /// \param num the number of keys
  /// \param x the key in question
  Result predecessor(const uint64_t x) const {
    if (tdc_unlikely(!m_min.exists)) return Result{false, 1};
    if (tdc_unlikely(x < m_min.pos)) return Result{false, 0};
    if (tdc_unlikely(x >= m_max.pos)) return Result{true, m_max};
    return {true, m_xf[prefix(x)]->pred(suffix(x))};
  }
};  // namespace dynamic

}  // namespace dynamic
}  // namespace pred
}  // namespace tdc
