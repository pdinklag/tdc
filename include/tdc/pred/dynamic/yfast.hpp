#include <robin_hood.h>

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <tdc/pred/result.hpp>
#include <vector>

namespace tdc {
namespace pred {
namespace dynamic {

// This class is a classical yfast-trie with practical improvements.
// It maintains a dynamic set of keys and answers predecessor queries.
// It needs O(n) space, needs O(1) time for key insertion/removal, and answers predecessor queries 
// in O(log log u) time, where u is the universe size. The data structure consists of two parts:
//  - The xfast_trie manages every '2^t_bucket_width'-th key. These keys are called a representants.
//  - A representant is the smallest key of the bucket it represents. A bucket holds ~2^t_bucket_width keys.  
//    Each bucket stores a pointer to its next smaller and next greater bucket.
// Each node in the xfast-trie contains a pointer and the nodes of the xfast trie are representated by an unordered_map in this way:
//  - The map contains the prefix <=> the node exists
//    - The node has two children <=> The prefix is mapped to nullptr
//    - The node has only a left child <=> The prefix is mapped to the greatest bucket in the left subtree
//    - The node has only a right child <=> The prefix is mapped to the smallest bucket in the right subtree
//    - The node has no children <=> The prefix is mapped to the only bucket whose representant has the same prefix as the node
// Template parameters:
//  - t_value_type is the type of the managed keys.
//  - t_key_width is the width of the managed keys.
//  - t_merge_threshhold indicates that a bucket is merged at size <= 2^(t_bucket_width/t_merge_threshold)

template <typename t_value_type, uint8_t t_key_width, uint8_t t_bucket_width, uint8_t t_merge_threshold = 2>
class YFastTrie {
 private:
  // Forward declaration because xfast_update and yfast_bucket are referencing each other
  class yfast_bucket;
  
  // Whenever we insert/delete, the bucket returns what has to be changed in the xfast_trie.
  // These updated to the xfast_trie are then applied. 
  struct xfast_update {
    std::vector<uint64_t> remove_repr; // The representants that have be removed from the xfast_trie.
    std::vector<yfast_bucket*> insert_repr; // The representants that have be inserted into the xfast_trie.
    std::vector<yfast_bucket*> delete_bucket; // The buckets that have to be deleted.
  };

  // The top part of the yfast_trie is the xfast_trie. It contains a unordered_map for every level.
  // The map for level l contains nodes with prefixes of the representants of length t_key_width-l.
  // A level maps a node to a bucket in the following way:
  //  - The map contains the prefix <=> the node exists
  //    - The node has two children <=> The prefix is mapped to nullptr
  //    - The node has only a left child <=> The prefix is mapped to the greatest bucket in the left subtree
  //    - The node has only a right child <=> The prefix is mapped to the smallest bucket in the right subtree
  //    - The node has no children <=> The prefix is mapped to the only bucket whose representant has the same prefix as the node
  std::array<robin_hood::unordered_map<uint64_t, yfast_bucket*>, t_key_width + 1> m_xfast;
  
  // The bottom part of the yfast_trie consists of yfast_buckets. A bucket stores a pointer to the next smaller and next greater bucket.
  // A bucket stores its keys in an unordered array. A bucket splits if it reaches size 2^(t_bucket_width+1). A bucket merges with its 
  // neighbor if if surpasses size 2^(t_bucket_width/t_merge_threshold). Whenevery something is inserted/deleted from a bucket, it
  // modifies the bottom structure on its own. It then returns which changes have to be made to the top data structure
  class yfast_bucket {
   private:
    static constexpr size_t c_bucket_size = 1ULL << t_bucket_width; // Approx. size of the bucket.
    yfast_bucket* m_prev = nullptr; // Pointer to the next smaller bucket.
    yfast_bucket* m_next = nullptr; // Pointer to the next greater bucket.
    t_value_type m_min; // The smallest key in the bucket. This is the representant.
    std::vector<t_value_type> m_elem; // The keys in stored by the bucket without the representant.

   public:
    yfast_bucket(t_value_type repr, yfast_bucket* pred, yfast_bucket* next) {
      m_min = repr;
      m_prev = pred;
      m_next = next;
    }

    // Returns the next smaller bucket.
    yfast_bucket* get_prev() const { return m_prev; }
    // Returns the next greater bucket.
    yfast_bucket* get_next() const { return m_next; }
    // Return the representant.
    t_value_type get_repr() const { return m_min; }

    // Inserts the key into the bucket and return the chages that have to be done
    // to the xfast_trie. A few things may lead changes in the top structure:
    xfast_update insert(const t_value_type x) {
      xfast_update update;
      // The key is smaller than the currenct representant. We swap these two and
      // mark that this has to be adressed in the xfast_trie.
      if (x < m_min) {
        update.remove_repr.push_back(static_cast<uint64_t>(m_min));
        update.insert_repr.push_back(this);
        m_elem.push_back(m_min);
        m_min = x;
      } else {
        m_elem.push_back(x);
      }
      // The buckets is too large, we split it in half and mark that the newly added
      // bucket has to be inserted into the xfast_trie
      if (m_elem.size() >= 2 * c_bucket_size) {
        update.insert_repr.push_back(split());
      }
      return update;
    }

    // Splits the 2*c_bucket_size big bucket into two equal sized ones 
    // and returns the pointer to the new one, so it can be inserted into the xfast_trie
    //TODO: split more efficiently by calculating an approx median.
    yfast_bucket* split() {
      std::sort(m_elem.begin(), m_elem.end());

      // Construct next bucket and adjust pointers.
      const size_t mid_index = m_elem.size() / 2;
      yfast_bucket* next_b = new yfast_bucket(m_elem[mid_index], this, m_next);
      if (m_next != nullptr) {
        m_next->m_prev = next_b;
      }
      m_next = next_b;

      // Move elements bigger than the median into the new bucket.
      for (size_t i = mid_index + 1; i < m_elem.size(); ++i) {
        next_b->insert(m_elem[i]);
      }
      m_elem.resize(mid_index);

      // Return pointer to newly added bucket.
      return next_b;
    }

    // Removes the key from the bucket and return the chages that have to be done
    // to the xfast_trie. The key must exist. A few things may lead changes in the top structure:
    std::vector<xfast_update> remove(const t_value_type x) {
      std::vector<xfast_update> updates;
      // If the key is the last element, we mark that this representant has to be deleted. 
      // The bucket also has to be deleted.
      if (m_elem.size() == 0) {
        if (m_prev != nullptr) {
          m_prev->m_next = m_next;
        }
        if (m_next != nullptr) {
          m_next->m_prev = m_prev;
        }
        updates.push_back(xfast_update());
        updates.back().remove_repr.push_back(static_cast<uint64_t>(x));
        updates.back().delete_bucket.push_back(this);
        return updates;
      }
      // Remove the element. If it was the representant, we choose the new representant and
      // mark that this has to be adressed in the xfast_trie.
      if (x == m_min) {
        updates.push_back(xfast_update());
        updates.back().remove_repr.push_back(static_cast<uint64_t>(m_min));
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

      // If the bucket is too small, we merge it with a neighbor. Here a representant has to
      // be removed from the top data structure. It may happen that the bucket we merged with becomes
      // too large. Then another split happens, and these changes also have to be applied to
      // the top data structure.
      if (m_elem.size() <= (c_bucket_size >> t_merge_threshold)) {
        std::vector<xfast_update> merge_updates = merge();
        updates.insert(updates.end(), merge_updates.begin(), merge_updates.end());
      }
      return updates;
    }

    // Merges the bucket with one of its neightbors. It may happen that the bucket we merged with becomes
    // too large. Then another split happens, and these changes also have to be applied to
    // the top data structure.
    std::vector<xfast_update> merge() {
      std::vector<xfast_update> updates;
      // First try to merge with the previous bucket.
      if (m_prev != nullptr) {
        // Adjust the pointers
        m_prev->m_next = m_next;
        if (m_next != nullptr) {
          m_next->m_prev = m_prev;
        }
        // Insert all elements into the previous bucket, except the representant
        for (size_t i = 0; i < m_elem.size(); ++i) {
          m_prev->m_elem.push_back(m_elem[i]);
        }
        // Mark that we need to remove the current representant and we need to delete the current bucket.
        updates.push_back(xfast_update());
        updates.back().remove_repr.push_back(static_cast<uint64_t>(m_min));
        updates.back().delete_bucket.push_back(this);
        // We insert the last element. Here a split may happen.
        updates.push_back(m_prev->insert(m_min));
      } else if (m_next != nullptr) {
        // Adjust the pointer
        m_next->m_prev = nullptr;
        // Here merge with the next_bucket.
        // Insert all elements into the previous bucket
        for (size_t i = 0; i < m_elem.size(); ++i) {
          m_next->m_elem.push_back(m_elem[i]);
        }
        // Mark that we need to remove the current representant and we need to delete the current bucket.
        updates.push_back(xfast_update());
        updates.back().remove_repr.push_back(static_cast<uint64_t>(m_min));
        updates.back().delete_bucket.push_back(this);
        // We insert the last element. Here a split may happen.
        updates.push_back(m_next->insert(m_min));
      }
      return updates;
    }

    // Returns the predecessor in the bucket.
    KeyResult<uint64_t> predecessor(t_value_type key) const {
      t_value_type max_pred = m_min;
      for (auto elem : m_elem) {
        if (elem <= key) {
          max_pred = std::max(elem, max_pred);
        }
      }
      return {true, static_cast<uint64_t>(max_pred)};
    }
  };

  // we don't need the lowest c_start_l levels;
  //TODO: skip the lowest levels in binary search
  static constexpr uint64_t c_start_l = 0;

  // The number of keys stored in this data structure
  size_t m_size = 0;

  // Insert a representant together with the bucket it represents
  // We traverse the tree from top to bot and update the tree until we find a 0-Node.
  // If there is a sibling, we can simply insert the representant. If the is no sibling, we have
  // to split the parent node.
  void insert_repr(uint64_t key, yfast_bucket* b) {
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
          // Child and sibling do not exist.
          // Here we have to split the node until we find the parent of child and node.
          yfast_bucket* smaller_bucket;
          yfast_bucket* greater_bucket;
          yfast_bucket* other_b = node->second;
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
          // Child does not exists, but sibling exists.
          // Here we can simply insert the new node and return.
          m_xfast[level][prefix] = nullptr;
          m_xfast[level - 1][child_prefix] = b;
          return;
        }
      } else {
        if (sibling == m_xfast[level - 1].end()) {
          // Child exists, but sibling does not exist
          // Here We may have to update the pointer of node
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
          //Both child and sibling exist
          //Here we do not have to do anything.
        }
      }
      //Prepare everything for the next iteration
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

  // Remove a representant from the tree.
  // We look for the leaf that points to the representant's bucket. This node must have a sibling
  // because there is no unary path to a leaf. If the sibling has any children, we simply remove
  // the representant. If the sibling does not have children, we to sift it up until it has a sibling
  // or becomes the root. From here we move to the top and adjust every pointer.
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
      // If the sibling does not have children we have to collapse the tree at this point.
      yfast_bucket* sift_up_bucket = sibling->second;
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
      // If the sibling has children we do not have to collapse the tree.
      ++level;
      last_bit = prefix & 0x1;
      prefix >>= 1;
      m_xfast[level][prefix] = (last_bit == 0) ? min_repr(level - 1, sibling_prefix) : max_repr(level - 1, sibling_prefix);
    }
    ++level;
    last_bit = prefix & 0x1;
    prefix = (level == 64) ? 0 : (key >> level);

    // From here on we traverse the tree up to the root any update pointers when a node only has one child.
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

  // Returns the bucket with the smallest representant in the subtree from the node
  // that corrospondents to (level, key).
  yfast_bucket* min_repr(uint64_t level, uint64_t key) const {
    while (level > c_start_l) {
      if (m_xfast[level - 1].find(key << 1) == m_xfast[level - 1].end()) {
        // Shortcut if there already is a pointer to the smallest representant.
        return m_xfast[level].at(key);
      }
      --level;
      key <<= 1;
    }
    return m_xfast[c_start_l].at(key);
  }
  // Returns the bucket with the biggest representant in the subtree from the node
  // that corrospondents to (level, key)
  yfast_bucket* max_repr(uint64_t level, uint64_t key) const {
    while (level > c_start_l) {
      if (m_xfast[level - 1].find((key << 1) + 1) == m_xfast[level - 1].end()) {
        // Shortcut if there already is a pointer to the biggest representant.
        return m_xfast[level].at(key);
      }
      --level;
      key = (key << 1) + 1;
    }
    return m_xfast[c_start_l].at(key);
  }

  // Return the lowest level at which a node representates a prefix of the key.
  size_t find_level(uint64_t key) const {
    // Binary search on the levels
    size_t l = c_start_l;
    size_t r = t_key_width;
    size_t m = (l + r) / 2;

    // The borders l and r meet at the lowest level in which a node with a prefix of key exist.
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

  // We search for the 1-node with a prefix of key that has the first 0->1 transition.
  // This node points either to the next smaller bucket or the next
  // greater bucket. If there is no next smaller bucket and t_nullptr_allowed,
  // we return a nullptr. If !t_nullptr_allowed we return the next greater bucket.
  template <bool t_nullptr_allowed>
  yfast_bucket* pred_bucket(uint64_t key) const {
    size_t first1 = find_level(key);
    yfast_bucket* b = m_xfast[first1].at((key >> 1) >> (first1 - 1));

    if constexpr (t_nullptr_allowed) {
      if (b->get_repr() <= key) {
        return b;
      } else {
        return b->get_prev();
      }
    } else {
      if (b->get_repr() <= key || (b->get_prev() == nullptr)) {
        return b;
      } else {
        return b->get_prev();
      }
    }
  }

 public:
  YFastTrie(){};
  // Inserts the first num keys from keys into the yfast_trie.
  YFastTrie(const uint64_t* keys, const size_t num) {
    for (size_t i = 0; i < num; ++i) {
      insert(keys[i]);
    }
  }

  // We search for the last_bucket and delete every bucket by following the pointers.
  ~YFastTrie() {
    if (m_size != 0) {
      yfast_bucket* b = max_repr(t_key_width, 0);
      yfast_bucket* p;
      while (b != nullptr) {
        p = b->get_prev();
        delete b;
        b = p;
      }
    }
  }

  // Returns the number of keys stored in the yfast_trie.
  size_t size() const {
    //std::cout << *this;
    return m_size;
  }

  // Insert key in data structure. This key must not be contained.
  void insert(uint64_t key) {
    ++m_size;

    // If is the first element, we insert the new bucket.
    if (tdc_unlikely(m_size == 1)) {
      yfast_bucket* new_b = new yfast_bucket(key, nullptr, nullptr);
      m_xfast[t_key_width][0] = new_b;
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
      insert_repr((uint64_t)((yfast_bucket*)new_bucket)->get_repr(), (yfast_bucket*)new_bucket);
    }
  }

  // Removes the key from data structure. This key must exist.
  void remove(uint64_t key) {
    --m_size;
    yfast_bucket* p_bucket = pred_bucket<true>(key);
    // Bucket returns which representants have to be changed.
    std::vector<xfast_update> updates = p_bucket->remove(key);
    // Here we update the xfast_trie.
    for (auto update : updates) {
      for (auto old_repr : update.remove_repr) {
        remove_repr(old_repr);
      }
      for (auto new_bucket : update.insert_repr) {
        insert_repr((uint64_t)((yfast_bucket*)new_bucket)->get_repr(), (yfast_bucket*)new_bucket);
      }
      for (auto delete_bucket : update.delete_bucket) {
        delete (yfast_bucket*)delete_bucket;
      }
    }
  }

  // Return the predecessor of key. If there are no keys {false, 0} is returned.
  // If there is no predecessor {false, 1} is returned.
  KeyResult<uint64_t> predecessor(uint64_t key) const {
    if (m_size == 0) {
      return {false, 0};
    }
    // Search the bucket that must contain the predecessor.
    yfast_bucket const * const search_bucket = pred_bucket<true>(key);
    if(search_bucket) {
      // If the bucket exist, search for the predecessor in the bucket and return it.
      return search_bucket->predecessor(key);
    } else {
      // If there is no predecessor, {false, 1} is returned.
      return {false, 1};
    }
  }

  // Print out basic information about the yfast_trie. If the trie is very small (less than 8 level),
  // give a graphic output of the xfast_trie.
  friend std::ostream& operator<<(std::ostream& o, const YFastTrie& yfast) {
    // General information
    size_t x = t_key_width;
    size_t y = t_bucket_width;
    o << "size: " << yfast.size() << "  word width: " << x << "  bucket width: " << y << std::endl;
    for (uint64_t i = 0; i <= x; ++i) {
      std::cout << "Level " << i << ": " << yfast.m_xfast[i].size() << std::endl;
    }
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
};
}  // namespace dynamic
}  // namespace pred
}  // namespace tdc