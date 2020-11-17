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

template <typename t_value_type, uint8_t t_bucket_width, uint8_t t_merge_threshold>
struct yfast_bucket {
  static constexpr size_t c_bucket_size = 1ULL << t_bucket_width;
  yfast_bucket* m_prev = nullptr;
  yfast_bucket* m_next = nullptr;
  t_value_type m_min;
  std::vector<t_value_type> m_elem;

  yfast_bucket(t_value_type repr, yfast_bucket* pred, yfast_bucket* next) {
    m_min = repr;
    m_prev = pred;
    m_next = next;
  }

  size_t size() const { return m_elem.size(); }

  t_value_type get_repr() const { return m_min; }

  xfast_update insert(const t_value_type x) {
    xfast_update update;
    if (x < m_min) {
      update.remove_repr.push_back((uint64_t)m_min);
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
    const size_t mid_index = m_elem.size() / 2;
    yfast_bucket* next_b = new yfast_bucket(m_elem[mid_index], this, m_next);
    if (m_next != nullptr) {
      m_next->m_prev = next_b;
    }
    m_next = next_b;

    // move elements bigger than the median into the new bucket
    for (size_t i = mid_index + 1; i < m_elem.size(); ++i) {
      next_b->insert(m_elem[i]);
    }
    m_elem.resize(mid_index);

    // return new bucket
    return next_b;
  }

  std::vector<xfast_update> remove(const t_value_type x) {
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
      updates.back().remove_repr.push_back((uint64_t)x);
      updates.back().delete_bucket.push_back(this);
      return updates;
    }
    //remove the element, if it was the representant,
    //choose the new representant
    if (x == m_min) {
      updates.push_back(xfast_update());
      updates.back().remove_repr.push_back((uint64_t)m_min);
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
    //TODO: INSERT MERGE AGAIN
    //if the bucket is too small, merge
    //if (m_elem.size() <= (c_bucket_size >> t_merge_threshold)) {
    //  std::vector<xfast_update> merge_updates = merge();
    //  updates.insert(updates.end(), merge_updates.begin(), merge_updates.end());
    //}
    return updates;
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
      updates.back().remove_repr.push_back((uint64_t)m_min);
      updates.back().delete_bucket.push_back(this);
      updates.push_back(m_prev->insert(m_min));
      assert((updates.back().insert_repr.size() <= 1) && (updates.back().remove_repr.size() == 0));
    } else if (m_next != nullptr) {
      for (size_t i = 0; i < m_elem.size(); ++i) {
        m_next->m_elem.push_back(m_elem[i]);
      }
      m_next->m_prev = nullptr;
      updates.push_back(xfast_update());
      updates.back().remove_repr.push_back((uint64_t)m_min);
      updates.back().delete_bucket.push_back(this);
      updates.push_back(m_next->insert(m_min));
    }
    return updates;
  }

  //returns predecessor in bucket.
  //it is required that m_min <= key
  KeyResult<uint64_t> predecessor(t_value_type key) const {
    if (tdc_unlikely(key < m_min)) {
      return {false, 0};
    }
    t_value_type max_pred = m_min;
    for (auto elem : m_elem) {
      if (elem <= key) {
        max_pred = std::max(elem, max_pred);
      }
    }
    return {true, (uint64_t)max_pred};
  }
};  // namespace dynamic

template <template <typename, uint8_t, uint8_t> typename t_bucket, typename t_value_type, uint8_t t_key_width, uint8_t t_bucket_width, uint8_t t_merge_threshold = 2, uint8_t t_start_l = t_bucket_width - t_merge_threshold>
class YFastTrie {
 private:
  using bucket = t_bucket<t_value_type, t_bucket_width, t_merge_threshold>;
  // we don't need the lowest c_start_l levels;
  static constexpr uint64_t c_start_l = t_start_l;
  size_t m_size = 0;

  std::array<robin_hood::unordered_map<uint64_t, bucket*>, t_key_width + 1> m_xfast;
  // Traverse and update the tree until we find a 0-Node. Here we have to split.
  void insert_repr(uint64_t key, bucket* b) {
    //Search for the first leaf and insert the node there
    int64_t level = t_key_width;
    uint64_t prefix = (key >> 1) >> (level - 1);  //this is equal to (key >> level)
    uint64_t next_bit = (key >> (level - 1)) & 0x1;
    uint64_t child_prefix = (prefix << 1) + next_bit;
    uint64_t sibling_prefix = (prefix << 1) + (1 - next_bit);

    auto node = m_xfast[level].find(prefix);
    auto child = m_xfast[level - 1].find(child_prefix);
    auto sibling = m_xfast[level - 1].find(sibling_prefix);

    while (level >= 0) {
      if (child == m_xfast[level - 1].end()) {
        if (sibling == m_xfast[level - 1].end()) {
          //child and sibling do not exist
          //split and return
          bucket* smaller_bucket;
          bucket* greater_bucket;
          bucket* other_b = node->second;
          uint64_t other_key = other_b->get_repr();
          uint64_t other_next_bit = (other_key >> (level - 1)) & 0x1;
          if (key < other_key) {
            smaller_bucket = b;
            greater_bucket = other_b;
          } else {
            smaller_bucket = other_b;
            greater_bucket = b;
          }
          while (next_bit == other_next_bit) {
            if (next_bit == 0) {
              m_xfast[level][prefix] = greater_bucket;
            } else {
              m_xfast[level][prefix] = smaller_bucket;
            }
            --level;
            prefix = (key >> level);
            next_bit = (key >> (level - 1)) & 0x1;
            other_next_bit = (other_key >> (level - 1)) & 0x1;
          }
          m_xfast[level][prefix] == nullptr;
          m_xfast[level - 1][prefix << 1] = smaller_bucket;
          m_xfast[level - 1][(prefix << 1) + 1] = greater_bucket;
          return;

        } else {
          //child does not exists, but sibling exists
          //insert node and return
          m_xfast[level][prefix] = nullptr;
          m_xfast[level - 1][child_prefix] = b;
          return;
        }
      } else {
        if (sibling == m_xfast[level - 1].end()) {
          // child exists, but sibling does not exist
          // we may have to update the pointer of node
          if (next_bit == 0) {
            if (b->get_repr() > node->second->get_repr()) {
              m_xfast[level][prefix] = b;
            }
          } else {
            if (b->get_repr() < node->second->get_repr()) {
              m_xfast[level][prefix] = b;
            }
          }
        } else {
          //both child and sibling exist
          //do nothing
        }
      }
      --level;
      prefix = (key >> level);
      next_bit = (key >> (level - 1)) & 0x1;
      child_prefix = (prefix << 1) + next_bit;
      sibling_prefix = (prefix << 1) + (1 - next_bit);

      node = child;
      child = m_xfast[level - 1].find(child_prefix);
      sibling = m_xfast[level - 1].find(sibling_prefix);
    }
  }

  void remove_repr(uint64_t key) {
    size_t level = find_level(key);
    uint64_t prefix = (level == 64) ? 0 : (key >> level);
    uint64_t last_bit = prefix & 0x1;
    m_xfast[level].erase(prefix);

    if (level == t_key_width) {
      return;
    }
    uint64_t sibling_prefix = (last_bit == 0) ? prefix + 1 : prefix - 1;
    auto sibling = m_xfast[level].find(sibling_prefix);
    auto sibling_left_child = m_xfast[level - 1].find((sibling_prefix << 1) + 0);
    auto sibling_right_child = m_xfast[level - 1].find((sibling_prefix << 1) + 1);

    if (sibling_left_child == m_xfast[level - 1].end() && sibling_right_child == m_xfast[level - 1].end()) {
      //if the sibling does not have children we have to colapse the tree at this point
      bucket* sift_up_bucket = sibling->second;
      m_xfast[level].erase(sibling_prefix);

      ++level;
      prefix >>= 1;
      last_bit = prefix & 0x1;
      sibling_prefix = (last_bit == 0) ? prefix + 1 : prefix - 1;
      sibling = m_xfast[level].find(sibling_prefix);

      while (sibling == m_xfast[level].end() && (level < t_key_width)) {
        m_xfast[level].erase(prefix);
        ++level;
        prefix >>= 1;
        last_bit = prefix & 0x1;
        sibling_prefix = (last_bit == 0) ? prefix + 1 : prefix - 1;
        sibling = m_xfast[level].find(sibling_prefix);
      }
      if (level == t_key_width) {
        m_xfast[level][0] = sift_up_bucket;
        return;
      }
      m_xfast[level][prefix] = sift_up_bucket;
      m_xfast[level + 1][prefix >> 1] = nullptr;
    } else {
      //if the sibling has children we do not have to colapse the tree
      ++level;
      last_bit = prefix & 0x1;
      prefix >>= 1;
      m_xfast[level][prefix] = (last_bit == 0) ? min_repr(level - 1, sibling_prefix) : max_repr(level - 1, sibling_prefix);
    }
    ++level;
    last_bit = prefix & 0x1;
    prefix = (level == 64) ? 0 : (key >> level);

    //from here on we traverse the tree up to the root any update pointers when a node only has one child
    while (level <= t_key_width) {
      uint64_t child_prefix = (prefix << 1) + last_bit;
      uint64_t other_child_prefix = (prefix << 1) + (1 - last_bit);
      auto other_child = m_xfast[level - 1].find(other_child_prefix);
      if (other_child == m_xfast[level - 1].end()) {
        m_xfast[level][prefix] = (last_bit == 0) ? max_repr(level - 1, child_prefix) : min_repr(level - 1, child_prefix);
      }
      ++level;
      last_bit = prefix & 0x1;
      prefix >>= 1;
    }
  }

  // returns the bucket with the smallest representant in the subtree from the node
  // that corrospondents to (level, key)
  bucket* min_repr(uint64_t level, uint64_t key) const {
    while (level > c_start_l) {
      if (m_xfast[level - 1].find(key << 1) == m_xfast[level - 1].end()) {
        return m_xfast[level].at(key);
      }
      --level;
      key <<= 1;
    }
    return m_xfast[c_start_l].at(key);
  }
  // returns the bucket with the biggest representant in the subtree from the node
  // that corrospondents to (level, key)
  bucket* max_repr(uint64_t level, uint64_t key) const {
    while (level > c_start_l) {
      if (m_xfast[level - 1].find((key << 1) + 1) == m_xfast[level - 1].end()) {
        return m_xfast[level].at(key);
      }
      --level;
      key = (key << 1) + 1;
    }
    return m_xfast[c_start_l].at(key);

    /*
    auto left_child = m_xfast[level-1].find((key << 1) + 0);
    auto right_child = m_xfast[level-1].find((key << 1) + 1);
    
    if(left_child == m_xfast[level-1].end()) {
      if(right_child == m_xfast[level-1].end()) {
        //left child and right child empty
        return m_xfast[level]
      }
    }
*/
  }

  // Return the highest level at which a node representates a prefix of key
  size_t find_level(uint64_t key) const {
    //binary search on the levels
    size_t l = c_start_l;
    size_t r = t_key_width;
    size_t m = (l + r) / 2;

    // l and r meet at the lowest 1
    while (l != r) {
      auto mapped = m_xfast[m].find(key >> m);
      if (mapped != m_xfast[m].end()) {
        r = m;
      } else {
        l = m + 1;
      }
      m = (l + r) / 2;
    }
    return r;
  }

  // For a key, we look at the node that has the first 0->1 transition.
  // This node points either to the next smaller bucket or the next
  // greater bucket. If there is no next smaller bucket and t_nullptr_allowed,
  // we return a nullptr. If t_nullptr_allowed we return the next greater bucket.
  template <bool t_nullptr_allowed>
  bucket* pred_bucket(uint64_t key) const {
    size_t first1 = find_level(key);
    bucket* b = m_xfast[first1].at((key >> 1) >> (first1 - 1));

    if constexpr (t_nullptr_allowed) {
      if (b->get_repr() <= key) {
        return b;
      } else {
        return b->m_prev;
      }
    } else {
      if (b->get_repr() <= key || (b->m_prev == nullptr)) {
        return b;
      } else {
        return b->m_prev;
      }
    }
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
  size_t size() const {
    /*
    std::cout << "DS size: " << m_size << std::endl; 
    for (uint64_t i = 0; i <= t_key_width; ++i) {
      std::cout << "Level " << i << ": " << m_xfast[i].size() << std::endl;
    }
    */
    return m_size;
  }

  // Insert key in data structure.
  void insert(uint64_t key) {
    ++m_size;

    // If is the first element, we insert the new bucket.
    if (tdc_unlikely(m_size == 1)) {
      bucket* new_b = new bucket(key, nullptr, nullptr);
      m_xfast[t_key_width][0] = new_b;
      //insert_repr(key, new_b);
      return;
    }
    // If it is not the first element, we search for the bucket in which the key belongs.
    auto b = pred_bucket<false>(key);
    // We insert the key in the bucket; the bucket says which representants have to be inserted/deleted.
    xfast_update xfu = b->insert(key);

    // Here we update the xfast_trie.
    for (auto old_repr : xfu.remove_repr) {
      remove_repr(old_repr);
    }
    for (auto new_bucket : xfu.insert_repr) {
      insert_repr((uint64_t)((bucket*)new_bucket)->get_repr(), (bucket*)new_bucket);
    }
  }

  //removes element from data structure
  void remove(uint64_t key) {
    --m_size;
    bucket* p_bucket = pred_bucket<true>(key);
    //bucket returns which representants have to be changed
    std::vector<xfast_update> updates = p_bucket->remove(key);
    // update xfast
    for (auto update : updates) {
      for (auto old_repr : update.remove_repr) {
        remove_repr(old_repr);
      }
      for (auto new_bucket : update.insert_repr) {
        insert_repr((uint64_t)((bucket*)new_bucket)->get_repr(), (bucket*)new_bucket);
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
    return pred_bucket<true>(key)->predecessor(key);
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
          std::cout << b->second->get_repr() << " ";
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