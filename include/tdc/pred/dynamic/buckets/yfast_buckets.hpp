#pragma once
#include <cstddef>
#include <cstdint>
#include <tdc/pred/result.hpp>

namespace tdc {
namespace pred {
namespace dynamic {

// Forward declaration because xfast_update and yfast_bucket are referencing each other
template <typename t_value_type, uint8_t t_bucket_width, uint8_t t_merge_threshold>
class yfast_bucket;
template <typename t_value_type, uint8_t t_bucket_width, uint8_t t_merge_threshold>
class yfast_bucket_sl;
// Whenever we insert/delete, the bucket returns what has to be changed in the xfast_trie.
// These updated to the xfast_trie are then applied.
template <typename t_bucket>
struct xfast_update {
  uint64_t repr_to_remove = 0;           // The representant that has be removed from the xfast_trie.
  t_bucket* repr_to_insert = nullptr;    // The representant that has be inserted into the xfast_trie.
  t_bucket* bucket_to_delete = nullptr;  // The bucket that has to be deleted.
};

// The bottom part of the yfast_trie consists of yfast_buckets. A bucket stores a pointer to the next smaller and next greater bucket.
// A bucket stores its keys in an unordered array. A bucket splits if it reaches size 2^(t_bucket_width+1). A bucket merges with its
// neighbor if if surpasses size 2^(t_bucket_width/t_merge_threshold). Whenevery something is inserted/deleted from a bucket, it
// modifies the bottom structure on its own. It then returns which changes have to be made to the top data structure

template <typename t_value_type, uint8_t t_bucket_width, uint8_t t_merge_threshold = 2>
class yfast_bucket {
 private:
  static constexpr size_t c_bucket_size = 1ULL << t_bucket_width;  // Approx. size of the bucket.
  yfast_bucket* m_prev;                                            // Pointer to the next smaller bucket.
  yfast_bucket* m_next;                                            // Pointer to the next greater bucket.
  t_value_type m_min;                                              // The smallest key in the bucket. This is the representant.
  bool m_repr_active;                                              // Says whether the representative is active
  std::vector<t_value_type> m_elem;                                // The keys in stored by the bucket without the representant.

 public:
  yfast_bucket(t_value_type repr, bool repr_active, yfast_bucket* prev, yfast_bucket* next)
      : m_min(repr), m_repr_active(repr_active), m_prev(prev), m_next(next) {
  }

  // Returns the next smaller bucket.
  yfast_bucket* get_prev() const { return m_prev; }
  // Return the representant.
  t_value_type get_repr() const { return m_min; }

  // Inserts the key into the bucket and return the chages that have to be done
  // to the xfast_trie. A bucket may become too large and split. Then we have
  // to insert a new representant into the x-fast trie.
  yfast_bucket* insert(const t_value_type key) {
    if (m_min == key) {
      // If the key is the inactive representant, we activate it.
      assert(m_repr_active == false);
      m_repr_active = true;
      return nullptr;
    }
    // Else we add the key
    m_elem.push_back(key);
    if (m_elem.size() >= 2 * c_bucket_size) {
      // The buckets is too large, we split it in half and return that the newly added
      // bucket has to be inserted into the xfast_trie
      return split();
    }
    return nullptr;
  }

  // Splits the 2*c_bucket_size big bucket into two equal sized ones
  // and returns the pointer to the new one, so it can be inserted into the xfast_trie
  yfast_bucket* split() {
    std::nth_element(m_elem.begin(), m_elem.begin() + m_elem.size() / 2, m_elem.end());

    // Construct next bucket and adjust pointers.
    const size_t mid_index = m_elem.size() / 2;
    yfast_bucket* next_b = new yfast_bucket(m_elem[mid_index], true, this, m_next);
    if (m_next != nullptr) {
      m_next->m_prev = next_b;
    }
    m_next = next_b;

    // Move elements bigger than the median into the new bucket.
    for (size_t i = mid_index + 1; i < m_elem.size(); ++i) {
      next_b->m_elem.push_back(m_elem[i]);
    }
    m_elem.resize(mid_index);

    // Return pointer to newly added bucket.
    return next_b;
  }

  // Removes the key from the bucket and return the chages that have to be done
  // to the xfast_trie. The key must exist. A few things may lead changes in the top structure:
  xfast_update<yfast_bucket> remove(const t_value_type key) {
    if (key == m_min) {
      assert(m_repr_active == true);
      m_repr_active = false;
      return xfast_update<yfast_bucket>{};
    }
    // Remove the key.
    auto del = std::find(m_elem.begin(), m_elem.end(), key);
    assert(del != m_elem.end());
    *del = m_elem.back();
    m_elem.pop_back();

    // If the bucket is too small, we merge it with the left neighbor.
    if (m_elem.size() <= (c_bucket_size >> t_merge_threshold)) {
      return merge();
    }
    return xfast_update<yfast_bucket>{};
  }

  // Merges the bucket with one of its neightbors. It may happen that the bucket we merged with becomes
  // too large. Then another split happens, and these changes also have to be applied to
  // the top data structure.
  xfast_update<yfast_bucket> merge() {
    xfast_update<yfast_bucket> update;
    if (m_prev == nullptr) {
      return xfast_update<yfast_bucket>{};
    }
    if (m_prev->m_elem.size() + m_elem.size() < 2 * c_bucket_size) {
      // Merge and delete this bucket
      // Adjust the pointers
      m_prev->m_next = m_next;
      if (m_next != nullptr) {
        m_next->m_prev = m_prev;
      }
      // Insert this representant if it is active.
      if (m_repr_active) {
        m_prev->m_elem.push_back(m_min);
      }
      // Insert all elements into the previous bucket, except the representant.
      for (size_t i = 0; i < m_elem.size(); ++i) {
        m_prev->m_elem.push_back(m_elem[i]);
      }
      update.repr_to_remove = static_cast<uint64_t>(m_min);
      update.bucket_to_delete = this;
    } else {
      // Merge-split
      // We don't have to update the pointers
      update.repr_to_remove = static_cast<uint64_t>(m_min);

      // Sort the previous bucket, because we want to take its biggest elements.
      std::vector<t_value_type>& prev_elem = m_prev->m_elem;
      const size_t element_count = m_elem.size() + prev_elem.size();
      const size_t mid_index = element_count / 2;

      std::nth_element(prev_elem.begin(), std::next(prev_elem.begin(), mid_index), prev_elem.end());
      if (m_repr_active) {
        m_elem.push_back(m_min);
      }
      // Put the biggest elements greater than the median from the previous bucket into this bucket.
      for (size_t i = mid_index + 1; i < prev_elem.size(); ++i) {
        m_elem.push_back(prev_elem[i]);
      }
      // Put the smallest element we take as the new representative.
      m_min = prev_elem[mid_index];
      m_repr_active = true;
      update.repr_to_insert = this;
      prev_elem.resize(mid_index);
    }
    return update;
  }

  // Returns the predecessor in the bucket.
  KeyResult<uint64_t> predecessor(t_value_type key) const {
    t_value_type max_pred = m_min;
    for (auto elem : m_elem) {
      if (elem <= key) {
        max_pred = std::max(elem, max_pred);
      }
    }
    if (max_pred != m_min || m_repr_active) {
      //We found a correct predecessor
      return {true, static_cast<uint64_t>(max_pred)};
    } else {
      if (m_prev != nullptr) {
        // The predecessor is in the previous bucket
        return m_prev->predecessor(key);
      } else {
        // There is not previous bucket and therefor no predecessor
        return {false, 0};
      }
    }
  }
};

template <typename t_value_type, uint8_t t_bucket_width, uint8_t t_merge_threshold = 2>
class yfast_bucket_sl {
 private:
  static constexpr size_t c_bucket_size = 1ULL << t_bucket_width;  // Approx. size of the bucket.
  yfast_bucket_sl* m_prev;                                         // Pointer to the next smaller bucket.
  yfast_bucket_sl* m_next;                                         // Pointer to the next greater bucket.
  t_value_type m_min;                                              // The smallest key in the bucket. This is the representant.
  bool m_repr_active;                                              // Says whether the representative is active
  std::vector<t_value_type> m_elem;                                // The keys in stored by the bucket without the representant.

 public:
  yfast_bucket_sl(t_value_type repr, bool repr_active, yfast_bucket_sl* prev, yfast_bucket_sl* next)
      : m_min(repr), m_repr_active(repr_active), m_prev(prev), m_next(next) {
  }

  // Returns the next smaller bucket.
  yfast_bucket_sl* get_prev() const { return m_prev; }
  // Return the representant.
  t_value_type get_repr() const { return m_min; }

  // Inserts the key into the bucket and return the chages that have to be done
  // to the xfast_trie. A bucket may become too large and split. Then we have
  // to insert a new representant into the x-fast trie.
  yfast_bucket_sl* insert(const t_value_type key) {
    if (m_min == key) {
      // If the key is the inactive representant, we activate it.
      assert(m_repr_active == false);
      m_repr_active = true;
      return nullptr;
    }
    // Else we add the key
    m_elem.push_back(0);
    size_t i = m_elem.size() - 1;
    while ((i != 0) && (m_elem[i - 1] >= key)) {
      m_elem[i] = m_elem[i - 1];
      --i;
    }
    m_elem[i] = key;
    if (m_elem.size() >= 2 * c_bucket_size) {
      // The buckets is too large, we split it in half and return that the newly added
      // bucket has to be inserted into the xfast_trie
      return split();
    }
    return nullptr;
  }

  // Splits the 2*c_bucket_size big bucket into two equal sized ones
  // and returns the pointer to the new one, so it can be inserted into the xfast_trie
  yfast_bucket_sl* split() {
    // Construct next bucket and adjust pointers.
    const size_t mid_index = m_elem.size() / 2;
    yfast_bucket_sl* next_b = new yfast_bucket_sl(m_elem[mid_index], true, this, m_next);
    if (m_next != nullptr) {
      m_next->m_prev = next_b;
    }
    m_next = next_b;

    // Move elements bigger than the median into the new bucket.
    for (size_t i = mid_index + 1; i < m_elem.size(); ++i) {
      next_b->m_elem.push_back(m_elem[i]);
    }
    m_elem.resize(mid_index);

    // Return pointer to newly added bucket.
    return next_b;
  }

  // Removes the key from the bucket and return the chages that have to be done
  // to the xfast_trie. The key must exist. A few things may lead changes in the top structure:
  xfast_update<yfast_bucket_sl> remove(const t_value_type key) {
    if (key == m_min) {
      assert(m_repr_active == true);
      m_repr_active = false;
      return xfast_update<yfast_bucket_sl>{};
    }
    // Remove the key.
    for (auto it = std::lower_bound(m_elem.begin(), m_elem.end(), key); it != m_elem.end() - 1; ++it) {
      *it = *(it + 1);
    }
    m_elem.pop_back();

    // If the bucket is too small, we merge it with the left neighbor.
    if (m_elem.size() <= (c_bucket_size >> t_merge_threshold)) {
      return merge();
    }
    return xfast_update<yfast_bucket_sl>{};
  }

  // Merges the bucket with one of its neightbors. It may happen that the bucket we merged with becomes
  // too large. Then another split happens, and these changes also have to be applied to
  // the top data structure.
  xfast_update<yfast_bucket_sl> merge() {
    xfast_update<yfast_bucket_sl> update;
    if (m_prev == nullptr) {
      return xfast_update<yfast_bucket_sl>{};
    }
    if (m_prev->m_elem.size() + m_elem.size() < 2 * c_bucket_size) {
      // Merge and delete this bucket
      // Adjust the pointers
      m_prev->m_next = m_next;
      if (m_next != nullptr) {
        m_next->m_prev = m_prev;
      }
      // Insert this representant if it is active.
      if (m_repr_active) {
        m_prev->m_elem.push_back(m_min);
      }
      // Insert all elements into the previous bucket, except the representant.
      for (size_t i = 0; i < m_elem.size(); ++i) {
        m_prev->m_elem.push_back(m_elem[i]);
      }
      update.repr_to_remove = static_cast<uint64_t>(m_min);
      update.bucket_to_delete = this;
    } else {
      // Merge-split
      // We don't have to update the pointers
      update.repr_to_remove = static_cast<uint64_t>(m_min);

      // Sort the previous bucket, because we want to take its biggest elements.
      std::vector<t_value_type>& prev_elem = m_prev->m_elem;
      const size_t element_count = m_elem.size() + prev_elem.size();
      const size_t mid_index = element_count / 2;

      // Put the biggest elements greater than the median from the previous bucket into this bucket.
      std::vector<t_value_type> new_m_elem;
      for (size_t i = mid_index + 1; i < prev_elem.size(); ++i) {
        new_m_elem.push_back(prev_elem[i]);
      }
      if (m_repr_active) {
        new_m_elem.push_back(m_min);
      }
      for (size_t i = 0; i < m_elem.size(); ++i) {
        new_m_elem.push_back(m_elem[i]);
      }
      std::swap(m_elem, new_m_elem);
      //Put the middle median key as the new representant
      m_min = prev_elem[mid_index];
      m_repr_active = true;
      update.repr_to_insert = this;
      //Adjust previous bucket, after elements were extracted
      prev_elem.resize(mid_index);
    }
    return update;
  }

  // Returns the predecessor in the bucket.
  KeyResult<uint64_t> predecessor(t_value_type key) const {
    t_value_type max_pred = m_min;
    auto it = std::upper_bound(m_elem.begin(), m_elem.end(), key);
    if (it != m_elem.begin()) {
      max_pred = *(it - 1);
    }
    if (max_pred != m_min || m_repr_active) {
      //We found a correct predecessor
      return {true, static_cast<uint64_t>(max_pred)};
    } else {
      if (m_prev != nullptr) {
        // The predecessor is in the previous bucket
        return m_prev->predecessor(key);
      } else {
        // There is not previous bucket and therefor no predecessor
        return {false, 0};
      }
    }
  }
};

}  // namespace dynamic
}  // namespace pred
}  // namespace tdc