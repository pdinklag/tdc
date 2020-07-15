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

// TODO: implement delete
// This is a bucket that holds a bit vector.
template <uint8_t b_wordl>
struct bucket_bv {
  uint64_t size = 0;
  uint64_t prefix;
  uint64_t prev_pred = 0;
  bucket_bv* next_b = nullptr;  // next bucket
  std::bitset<1ULL << b_wordl> bits;

  void set(const uint64_t i) {
    if (!bits[i]) {
      ++size;
      bits[i] = true;
    }
  }

  // deletes a key and updates the bottom level of the data structure
  // it can not update the top level
  bool del(uint64_t i) {
    /*
    if (bits[i]) {
      --size;
      bits[i] = false;

      // here we update next_b->prev_pred
      if (tdc_likely(next_b != nullptr)) {
        if (next_b->prev_pred == i) {
          if (size != 0) {
            while (!bits[i--]) {
            };
            next_b->prev_pred = (prefix << bwordl) + i;
          } else {
            next_b->prev_pred = pred_prev;
          }
        }
      }
      // if this bucket is deleted we have to update the pointer
      // in the previous bucket
      if(size == 0) {
        bucket* = m_xf[0];
      }
      return (size == 0);
      */
    return true;
  }

  uint64_t pred(int64_t i) const {
    for (; i >= 0; --i) {
      if (bits[i]) {
        return (prefix << b_wordl) + i;
      }
    }
    return prev_pred;
  }
};

// TODO: implement delete
// This is a bucket that holds an std::vector.
static constexpr size_t upper_threshhold = 1ULL << 10;
static constexpr size_t lower_threshhold = 1ULL << 8;
template <uint8_t b_wordl>
struct bucket_list {
  uint64_t size = 0;
  uint64_t prefix;
  uint64_t prev_pred = 0;
  bucket_list* next_b = nullptr;
  std::vector<uint16_t> list;

  void set(uint64_t i) {
    list.push_back(i);
    ++size;
  }

  bool del(uint64_t i) { return true; }
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

// TODO: implement delete
// This is a bucket that either holds an std::vector or a bit_vector depending
// on fill rate
template <uint8_t b_wordl>
struct bucket_hybrid {
  uint64_t size = 0;
  uint64_t prefix;
  uint64_t prev_pred = 0;
  bucket_hybrid* next_b = nullptr;

  bool is_list = true;
  std::vector<uint16_t> list;
  std::bitset<1ULL << b_wordl>* bits;

  ~bucket_hybrid() {
    if (!is_list) {
      delete bits;
    }
  }
  void set(const uint64_t i) {
    if (is_list) {
      list.push_back(i);
      ++size;
    } else {
      if (!(*bits)[i]) {
        ++size;
        (*bits)[i] = true;
      }
    }
    if (is_list && (size > upper_threshhold)) {
      rebuild();
    }
  }

  uint64_t pred(int64_t i) const {
    if (is_list) {
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
    } else {
      for (; i >= 0; --i) {
        if ((*bits)[i]) {
          return (prefix << b_wordl) + i;
        }
      }
      return prev_pred;
    }
  }

  void rebuild() {
    if (is_list) {
      is_list = false;
      size = 0;
      bits = new std::bitset<1ULL << b_wordl>;
      for (uint16_t key : list) {
        set(key);
      }
      //this frees the memory allocated by the vector
      std::vector<uint16_t>().swap(list); 
    } else {
      is_list = true;
      size = 0;
      for (size_t i = 0; i < (1ULL << b_wordl); ++i) {
        if ((*bits)[i]) {
          set(i);
        }
      }
      delete bits;
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
  static constexpr size_t wordl = 40;
  static constexpr size_t word_size = (1ULL << wordl);
  // The top part of the data structure stores pointers to the bottom buckets.
  static constexpr size_t x_wordl = 40 - t_sampling;
  static constexpr size_t x_size = (1ULL << x_wordl);
  // The bottom part of the data structure is either
  // a bit vector, std::array, or a hybrid.
  static constexpr size_t b_wordl = t_sampling;
  static constexpr size_t b_size = (1ULL << b_wordl);

  using bucket = t_list<b_wordl>;
  uint64_t m_size = 0;          // number of keys stored
  uint64_t m_min;               // stores the minimal key
  uint64_t m_max;               // stores the maximal key
  std::vector<bucket*> m_xf;    // top data structure
  bucket* m_first_b = nullptr;  // pointer to the first bucket

  // return the x_wordl more significant bits of i
  inline uint64_t prefix(uint64_t i) const { return i >> b_wordl; }
  // return the b_wordl less significant bits
  inline uint64_t suffix(uint64_t i) const { return i & (b_size - 1); }

 public:
  inline DynIndex() { m_xf.resize(0); }

  /// \brief Constructs the index for the given keys.
  /// \param keys a pointer to the keys, that must be in ascending order
  /// \param num the number of keys
  DynIndex(const uint64_t* keys, const size_t num) {
    assert_sorted_ascending(keys, num);
    for (size_t i = 0; i < num; ++i) {
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
    const uint64_t key_pre = prefix(key);
    const uint64_t key_suf = suffix(key);
    bucket* new_b;

    // if a key is inserted that needs a greater bucket
    if (key_pre >= m_xf.size()) {
      if (tdc_likely(m_size != 0)) {
        new_b = new bucket;
        new_b->prefix = key_pre;
        new_b->next_b = nullptr;
        new_b->set(key_suf);
        new_b->prev_pred = m_max;
        m_xf.back()->next_b = new_b;
        m_xf.resize(prefix(key) + 1, m_xf.back());
      } else {
        m_min = key;
        m_max = key;
        new_b = new bucket;
        new_b->prefix = key_pre;
        new_b->next_b = m_first_b;
        m_first_b = new_b;
        new_b->set(key_suf);
        ++m_size;  // TODO: CALCULATE SIZE CORRECTLY
        m_xf.resize(prefix(key) + 1, nullptr);
      }
    } else if (key_pre < prefix(m_min)) {
      // if a key is inserted before the first bucket
      m_first_b->prev_pred = key;
      new_b = new bucket;
      new_b->prefix = key_pre;
      new_b->prev_pred = 0;
      new_b->next_b = m_first_b;
      m_first_b = new_b;
      new_b->set(key_suf);
    } else {
      // if a key is inserted inbetween
      bucket* key_bucket = m_xf[key_pre];
      // if exact bucket exists, add key
      if (key_bucket->prefix == key_pre) {
        key_bucket->set(key_suf);
        bucket* next_bucket = key_bucket->next_b;
        if (next_bucket != nullptr) {
          next_bucket->prev_pred = std::max(key, next_bucket->prev_pred);
        }
        m_min = std::min(key, m_min);
        m_max = std::max(key, m_max);
        return;
      } else {
        // if exact bucket does not exist
        new_b = new bucket;
        new_b->prefix = key_pre;
        new_b->next_b = key_bucket->next_b;
        new_b->set(key_suf);
        // pointer in next_bucket
        key_bucket->next_b = new_b;
        new_b->prev_pred = new_b->next_b->prev_pred;
        new_b->next_b->prev_pred = key;
      }
    }

    m_min = std::min(key, m_min);
    m_max = std::max(key, m_max);
    // update xfast
    m_xf[key_pre] = new_b;
    if (key_pre + 1 < m_xf.size()) {
      bucket* next_bucket = m_xf[key_pre + 1];
      if (next_bucket == nullptr || next_bucket->prefix < key_pre) {
        for (size_t j = key_pre + 1; j < m_xf.size() && next_bucket == m_xf[j];
             ++j) {
          m_xf[j] = new_b;
        }
      }
    }
  }

  void del(const uint64_t key) {
    if (key < m_min && key > m_max) {
      return;
    }
    m_xf[prefix(key)]->del(suffix(key));
    // TODO: implement delete
  }
  /// \brief Finds the rank of the predecessor of the specified key.
  /// \param keys the keys that the compressed trie was constructed for
  /// \param num the number of keys
  /// \param x the key in question
  Result predecessor(const uint64_t x) const {
    if (tdc_unlikely(m_size == 0)) return Result{false, 1};
    if (tdc_unlikely(x < m_min)) return Result{false, 0};
    if (tdc_unlikely(x >= m_max)) return Result{true, m_max};
    return {true, m_xf[prefix(x)]->pred(suffix(x))};
  }
};
}  // namespace dynamic
}  // namespace pred
}  // namespace tdc
