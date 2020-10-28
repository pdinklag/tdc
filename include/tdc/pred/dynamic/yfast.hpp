#include <robin_hood.h>

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <tdc/pred/result.hpp>
#include <vector>

namespace tdc {
namespace pred {
namespace dynamic {

struct xfast_update {
  std::vector<uint64_t> remove_repr;
  std::vector<void*> insert_repr;
  std::vector<void*> delete_bucket;
};

template <uint8_t t_bucket_width, uint8_t t_merge_threshold>
struct yfast_bucket {
  static constexpr size_t c_bucket_size = 1ULL << t_bucket_width;
  yfast_bucket* m_prev = nullptr;
  yfast_bucket* m_next = nullptr;
  uint64_t m_min;
  std::vector<uint64_t> m_elem;

  yfast_bucket(uint64_t repr, yfast_bucket* pred, yfast_bucket* next) {
    m_min = repr;
    m_prev = pred;
    m_next = next;
  }

  size_t size() const { return m_elem.size(); }

  uint64_t get_repr() const { return m_min; }

  xfast_update insert(const uint64_t x) {
    xfast_update update;
    if (x < m_min) {
      update.remove_repr.push_back(m_min);
      update.insert_repr.push_back(this);
      m_elem.push_back(m_min);
      m_min = x;
    } else {
      m_elem.push_back(x);
    }
    if (m_elem.size() >= 2 * c_bucket_size) {
      update.insert_repr.push_back(split());
    }
    return update;
  }

  //splits the 2*c_bucket_size big bucket into two equal sized ones
  //and returns the pointer to the new one, so it can be inserted
  //into the xfast trie
  yfast_bucket* split() {
    std::sort(m_elem.begin(), m_elem.end());

    // construct next bucket and adjust pointers
    yfast_bucket* next_b = new yfast_bucket(m_elem[c_bucket_size], this, m_next);
    if (m_next != nullptr) {
      m_next->m_prev = next_b;
    }
    m_next = next_b;

    // move elements bigger than the median into the new bucket
    for (size_t i = c_bucket_size + 1; i < 2 * c_bucket_size; ++i) {
      next_b->insert(m_elem[i]);
    }
    m_elem.resize(c_bucket_size);

    // return new bucket
    return next_b;
  }

  std::vector<xfast_update> merge() {
    std::vector<xfast_update> updates;
    if (m_prev != nullptr) {
      m_prev->m_next = m_next;
      if (m_next != nullptr) {
        m_next->m_prev = m_prev;
      }
      for (size_t i = 0; i < m_elem.size(); ++i) {
        m_prev->m_elem.push_back(m_elem[i]);
      }
      updates.push_back(xfast_update());
      updates.back().remove_repr.push_back(m_min);
      updates.back().delete_bucket.push_back(this);
      updates.push_back(m_prev->insert(m_min));
      assert((updates.back().insert_repr.size() <= 1) && (updates.back().remove_repr.size() == 0));
    } else if (m_next != nullptr) {
      for (size_t i = 0; i < m_elem.size(); ++i) {
        m_next->m_elem.push_back(m_elem[i]);
      }
      m_next->m_prev = nullptr;
      updates.push_back(xfast_update());
      updates.back().remove_repr.push_back(m_min);
      updates.back().delete_bucket.push_back(this);
      updates.push_back(m_next->insert(m_min));
    }
    return updates;
  }

  std::vector<xfast_update> remove(const uint64_t x) {
    std::vector<xfast_update> updates;
    // if it is the last element
    if (size() == 0) {
      if (m_prev != nullptr) {
        m_prev->m_next = m_next;
      }
      if (m_next != nullptr) {
        m_next->m_prev = m_prev;
      }
      updates.push_back(xfast_update());
      updates.back().remove_repr.push_back(x);
      updates.back().delete_bucket.push_back(this);
      return updates;
    }

    //remove the element, if it was the representant,
    //choose the new representant
    if (x == m_min) {
      updates.push_back(xfast_update());
      updates.back().remove_repr.push_back(m_min);
      auto new_min = std::min_element(m_elem.begin(), m_elem.end());
      m_min = *new_min;
      *new_min = m_elem.back();
      m_elem.pop_back();
      updates.back().insert_repr.push_back(this);
    } else {
      auto del = std::find(m_elem.begin(), m_elem.end(), x);
      *del = m_elem.back();
      m_elem.pop_back();
    }

    //if the bucket is too small, merge
    if (m_elem.size() <= c_bucket_size / t_merge_threshold) {
      std::vector<xfast_update> merge_updates = merge();
      updates.insert(updates.end(), merge_updates.begin(), merge_updates.end());
    }
    return updates;
  }

  //returns predecessor in bucket.
  //it is required that m_min <= key
  KeyResult<uint64_t> predecessor(uint64_t key) const {
    if (tdc_unlikely(key < m_min)) {
      return {false, 0};
    }
    uint64_t max_pred = m_min;
    for (size_t i = 0; i < size(); ++i) {
      if (m_elem[i] <= key) {
        max_pred = std::max(m_elem[i], max_pred);
      }
    }
    return {true, max_pred};
  }
};  // namespace dynamic

template <template <uint8_t, uint8_t> class t_bucket, uint8_t t_key_width, uint8_t t_bucket_width, uint8_t t_merge_threshold = 4, bool t_bin_level_search = true>
class YFastTrie {
 private:
  using bucket = t_bucket<t_bucket_width, t_merge_threshold>;
  size_t m_size = 0;

  std::array<robin_hood::unordered_map<uint64_t, bucket*>, t_key_width + 1> m_xfast;

  void insert_repr(uint64_t key, bucket* b) {
    m_xfast[0][key] = b;

    uint64_t level = 1;
    uint64_t last_bit = key & 0x1;
    key >>= 1;
    // we first find zeroes
    while (level <= t_key_width && (m_xfast[level].find(key) == m_xfast[level].end())) {
      m_xfast[level][key] = b;
      ++level;
      last_bit = key & 0x1;
      key >>= 1;
    }
    if (level <= t_key_width) {
      m_xfast[level][key] = nullptr;
      ++level;
      last_bit = key & 0x1;
      key >>= 1;
    }
    // then we find ones; if one children is a zero, we update the pointer
    while (level <= t_key_width) {
      auto other_child = m_xfast[level - 1].find((key << 1) + (1 - last_bit));
      if (other_child == m_xfast[level - 1].end()) {
        if ((m_xfast[level][key]->get_repr() < b->get_repr()) ^ last_bit) {
          m_xfast[level][key] = b;
        }
        ++level;
        last_bit = key & 0x1;
        key >>= 1;
        continue;
      }
      // if both children are ones, we remove the pointer
      m_xfast[level][key] = nullptr;
      ++level;
      last_bit = key & 0x1;
      key >>= 1;
    }
  }

  void remove_repr(uint64_t key) {
    m_xfast[0].erase(key);

    uint64_t level = 1;
    uint64_t last_bit = key & 0x1;
    key >>= 1;
    // while the deleted key was the only representant in the subtree, we delete all nodes
    while (level <= t_key_width) {
      auto other_child = m_xfast[level - 1].find((key << 1) + (1 - last_bit));
      if (other_child == m_xfast[level - 1].end()) {
        m_xfast[level].erase(key);
      } else {
        m_xfast[level][key] = (last_bit == 0) ? min_repr(level - 1, (key << 1) + 1) : max_repr(level - 1, (key << 1));
        ++level;
        last_bit = key & 0x1;
        key >>= 1;
        break;
      }
      ++level;
      last_bit = key & 0x1;
      key >>= 1;
    }

    // from here on we have to update the pointer if there is one
    while (level <= t_key_width) {
      auto other_child = m_xfast[level - 1].find((key << 1) + (1 - last_bit));
      if (other_child == m_xfast[level - 1].end()) {
        m_xfast[level][key] = (last_bit == 0) ? max_repr(level - 1, (key << 1)) : min_repr(level - 1, (key << 1) + 1);
      }
      ++level;
      last_bit = key & 0x1;
      key >>= 1;
    }
  }

  // returns the bucket with the smallest representant in the subtree from the node
  // that corrospondents to (level, key)
  bucket* min_repr(uint64_t level, uint64_t key) const {
    while (level > 0) {
      if (m_xfast[level - 1].find(key << 1) == m_xfast[level - 1].end()) {
        return m_xfast[level].at(key);
      }
      --level;
      key <<= 1;
    }
    return m_xfast[0].at(key);
  }
  // returns the bucket with the biggest representant in the subtree from the node
  // that corrospondents to (level, key)
  bucket* max_repr(uint64_t level, uint64_t key) const {
    while (level > 0) {
      if (m_xfast[level - 1].find((key << 1) + 1) == m_xfast[level - 1].end()) {
        return m_xfast[level].at(key);
      }
      --level;
      key = (key << 1) + 1;
    }
    return m_xfast[0].at(key);
  }

 public:
  YFastTrie(){};
  YFastTrie(const uint64_t* keys, const size_t num) {
    for (size_t i = 0; i < num; ++i) {
      insert(keys[i]);
    }
  }

  ~YFastTrie() {
    if (m_size != 0) {
      //get bucket with maximal representative and delete every bucket from last to first
      bucket* b = max_repr(t_key_width, 0);
      bucket* p;
      while (b != nullptr) {
        p = b->m_prev;
        delete b;
        b = p;
      }
    }
  }
  size_t size() const { return m_size; }

  //inserts element in bucket. bucket return which representants have to be changed
  void insert(uint64_t key) {
    ++m_size;

    //if first element
    if (tdc_unlikely(m_size == 1)) {
      bucket* new_b = new bucket(key, nullptr, nullptr);
      insert_repr(key, new_b);
      return;
    }

    // search for the first 1,
    xfast_update xfu;
    uint64_t prefix = key;
    for (size_t level = 0; level <= t_key_width; ++level) {
      auto elem = m_xfast[level].find(prefix);
      if (elem != m_xfast[level].end()) {
        if (elem->second->m_min > key && elem->second->m_prev != nullptr) {
          xfu = elem->second->m_prev->insert(key);
          break;
        } else {
          xfu = elem->second->insert(key);
          break;
        }
      }
      prefix >>= 1;
    }
    // update xfast
    for (auto old_repr : xfu.remove_repr) {
      remove_repr(old_repr);
    }
    for (auto new_bucket : xfu.insert_repr) {
      insert_repr(((bucket*)new_bucket)->m_min, (bucket*)new_bucket);
    }
  }

  // For a key, returns the bucket in which the key must be
  bucket* pred_bucket(uint64_t key) const {
    //alternative: linear search
    if constexpr (t_bin_level_search) {
      auto repr_bucket = m_xfast[0].find(key);
      if (repr_bucket != m_xfast[0].end()) {
        return repr_bucket->second;
      }

      size_t l = 0;            //point to highest 0
      size_t r = t_key_width;  //points to lowest 1
      size_t m = (l + r) / 2;

      while (l + 1 != r) {
        auto mapped = m_xfast[m].find(key >> m);
        if (mapped != m_xfast[m].end()) {
          r = m;
        } else {
          l = m;
        }
        m = (l + r) / 2;
      }
      bucket* b = m_xfast[r].at((key >> 1) >> r - 1);

      if (((key >> l) & 1) == 1) {
        return b;
      } else {
        return b->m_prev;
      }

    } else {
      uint64_t prefix = key;
      bucket* b;
      size_t level = 0;
      while (level <= t_key_width) {
        auto mapped = m_xfast[level].find(prefix);
        if (mapped != m_xfast[level].end()) {
          b = mapped->second;
          break;
        }
        ++level;
        prefix >>= 1;
      }
      if (((key >> (level - 1)) & 1) == 1) {
        return b;
      } else {
        return b->m_prev;
      }
    }
  }

  //removes element from data structure
  void remove(uint64_t key) {
    --m_size;
    bucket* p_bucket = pred_bucket(key);
    //bucket returns which representants have to be changed
    std::vector<xfast_update> updates = p_bucket->remove(key);
    // update xfast
    for (auto update : updates) {
      for (auto old_repr : update.remove_repr) {
        remove_repr(old_repr);
      }
      for (auto new_bucket : update.insert_repr) {
        insert_repr(((bucket*)new_bucket)->m_min, (bucket*)new_bucket);
      }
      for (auto delete_bucket : update.delete_bucket) {
        delete (bucket*)delete_bucket;
      }
    }
  }

  KeyResult<uint64_t> predecessor(uint64_t key) const {
    if (m_size == 0) {
      return {false, 0};
    }
    return pred_bucket(key)->predecessor(key);
  }

  friend std::ostream& operator<<(std::ostream& o, const YFastTrie& yfast) {
    // General information
    size_t x = t_key_width;
    size_t y = t_bucket_width;
    o << "size: " << yfast.size() << "  word width: " << x << "  bucket width: " << y << std::endl;

    // xfast
    if (t_key_width < 8) {
      uint64_t line_length = 1ULL << t_key_width;

      // print tree
      for (int8_t l = t_key_width; l >= 0; --l) {
        uint64_t line_pause = line_length >> (t_key_width - l);

        // before first node
        for (uint64_t sp = 1; sp < (line_pause / 2); ++sp) {
          std::cout << " ";
        }
        // print nodes
        for (uint64_t n = 0; n < (1ULL << (t_key_width - l)); ++n) {
          std::cout << (yfast.m_xfast[l].find(n) != yfast.m_xfast[l].end());
          for (uint64_t sp = 1; sp < line_pause; ++sp) {
            std::cout << " ";
          }
        }
        std::cout << '\n';
      }
      //print buckets
      for (size_t i = 0; i < 1ULL << (t_key_width); ++i) {
        auto b = yfast.m_xfast[0].find(i);
        if (b != yfast.m_xfast[0].end()) {
          std::cout << b->second->m_min << " ";
          for (size_t j = 0; j < b->second->size(); ++j) {
            std::cout << b->second->m_elem[j] << " ";
          }
          std::cout << std::endl;
        }
      }
    }
    return o;
  }
};  // namespace dynamic
}  // namespace dynamic
}  // namespace pred
}  // namespace tdc