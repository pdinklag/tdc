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
    uint64_t repr_to_remove = 0;         // The representants that have be removed from the xfast_trie.
    yfast_bucket* repr_to_insert = nullptr;    // The representants that have be inserted into the xfast_trie.
    yfast_bucket* bucket_to_delete = nullptr;  // The buckets that have to be deleted.
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
        next_b->insert(m_elem[i]);
      }
      m_elem.resize(mid_index);

      // Return pointer to newly added bucket.
      return next_b;
    }

    // Removes the key from the bucket and return the chages that have to be done
    // to the xfast_trie. The key must exist. A few things may lead changes in the top structure:
    xfast_update remove(const t_value_type key) {
      if (key == m_min) {
        assert(m_repr_active == true);
        m_repr_active = false;
        return xfast_update{};
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
      return xfast_update{};
    }

    // Merges the bucket with one of its neightbors. It may happen that the bucket we merged with becomes
    // too large. Then another split happens, and these changes also have to be applied to
    // the top data structure.
    xfast_update merge() {
      xfast_update update;
      if (m_prev == nullptr) {
        return xfast_update{};
      }
      if (m_prev->m_elem.size() + m_elem.size() < 2 * c_bucket_size) {
        // Merge and delete this bucket
        // Adjust the pointers
        m_prev->m_next = m_next;
        if (m_next != nullptr) {
          m_next->m_prev = m_prev;
        }
        // Insert all elements into the previous bucket, except the representant.
        for (size_t i = 0; i < m_elem.size(); ++i) {
          m_prev->m_elem.push_back(m_elem[i]);
        }
        // Insert this representant if it is active.
        if (m_repr_active) {
          m_prev->m_elem.push_back(m_min);
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
        // Put the smallest element we take as the new representative.
        m_min = prev_elem[mid_index];
        m_repr_active = true;
        update.repr_to_insert = this;
        // Put the biggest elements greater than the median from the previous bucket into this bucket.
        for (size_t i = mid_index + 1; i < prev_elem.size(); ++i) {
          m_elem.push_back(prev_elem[i]);
        }
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

  // When searching for a bucket we can speed up the binary search in the xfast_trie by remembering
  //  - the lowest level where every node exists
  //  - the lowest level where at least one node exists
  uint64_t m_lowest_complete_level = t_key_width;
  uint64_t m_lowest_non_empty_level = t_key_width;

  // The number of keys stored in this data structure
  size_t m_size = 0;

  // Insert a representant together with the bucket it represents
  // We traverse the tree from top to bot and update the tree until we find a 0-Node.
  // If there is a sibling, we can simply insert the representant. If the is no sibling, we have
  // to split the parent node.
  void insert_repr(yfast_bucket* const b) {

    const uint64_t key = static_cast<uint64_t>(b->get_repr());
    uint64_t level = t_key_width;
    uint64_t prefix = (key >> 1) >> (level - 1);  //this is equal to (key >> level)
    uint64_t next_bit = (key >> (level - 1)) & 0x1;
    uint64_t child_prefix = (prefix << 1) + next_bit;
    uint64_t sibling_prefix = (prefix << 1) + (1 - next_bit);

    auto node = m_xfast[level].find(prefix);
    auto child = m_xfast[level - 1].find(child_prefix);
    auto sibling = m_xfast[level - 1].find(sibling_prefix);

    while (level > 0) {
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
          // Here we may have to update the pointer of node
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

  void update_after_insertion() {
    while ((m_lowest_complete_level > 0) && (m_xfast[m_lowest_complete_level - 1].size() == (1ULL << 1 << (64 - m_lowest_complete_level)))) {
      --m_lowest_complete_level;
    }
    while (m_lowest_non_empty_level > 0 && (m_xfast[m_lowest_non_empty_level - 1].size() > 0)) {
      --m_lowest_non_empty_level;
    }
  }
  void update_after_deletion() {
    while ((m_lowest_complete_level < t_key_width) && (m_xfast[m_lowest_complete_level].size() < (1ULL << (64 - m_lowest_complete_level)))) {
      ++m_lowest_complete_level;
    }
    while ((m_lowest_non_empty_level < t_key_width) && (m_xfast[m_lowest_non_empty_level].size() == 0)) {
      ++m_lowest_non_empty_level;
    }
  }

  // Returns the bucket with the smallest representant in the subtree from the node
  // that corrospondents to (level, key).
  yfast_bucket* min_repr(uint64_t level, uint64_t key) const {
    while (level > 0) {
      if (m_xfast[level - 1].find(key << 1) == m_xfast[level - 1].end()) {
        // Shortcut if there already is a pointer to the smallest representant.
        return m_xfast[level].at(key);
      }
      --level;
      key <<= 1;
    }
    return m_xfast[0].at(key);
  }
  // Returns the bucket with the biggest representant in the subtree from the node
  // that corrospondents to (level, key)
  yfast_bucket* max_repr(uint64_t level, uint64_t key) const {
    while (level > 0) {
      if (m_xfast[level - 1].find((key << 1) + 1) == m_xfast[level - 1].end()) {
        // Shortcut if there already is a pointer to the biggest representant.
        return m_xfast[level].at(key);
      }
      --level;
      key = (key << 1) + 1;
    }
    return m_xfast[0].at(key);
  }

  // Return the lowest level at which a node representates a prefix of the key.
  size_t find_level(uint64_t key) const {
    // Binary search on the levels
    size_t l = m_lowest_non_empty_level;
    size_t r = m_lowest_complete_level;
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
  // greater bucket. We return the next smaller bucket or the predecessor of the
  // next greater bucket.
  yfast_bucket* pred_bucket(uint64_t key) const {
    size_t first1 = find_level(key);
    yfast_bucket* b = m_xfast[first1].at((key >> 1) >> (first1 - 1));
    if (b->get_repr() <= key) {
      return b;
    } else {
      return b->get_prev();
    }
  }

 public:
  YFastTrie() {
    m_xfast[t_key_width][0] = new yfast_bucket(0, false, nullptr, nullptr);
  };
  // Inserts the first num keys from keys into the yfast_trie.
  YFastTrie(const uint64_t* keys, const size_t num) {
    m_xfast[t_key_width][0] = new yfast_bucket(0, false, nullptr, nullptr);
    for (size_t i = 0; i < num; ++i) {
      insert(keys[i]);
    }
  }

  // We search for the last_bucket and delete every bucket by following the pointers.
  ~YFastTrie() {
    yfast_bucket* b = max_repr(t_key_width, 0);
    yfast_bucket* p;
    while (b != nullptr) {
      p = b->get_prev();
      delete b;
      b = p;
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
    // We search for the bucket in which the key belongs.
    auto b = pred_bucket(key);
    // We insert the key in the bucket; the bucket says which representant has to be inserted.
    yfast_bucket* new_bucket = b->insert(key);
    if (new_bucket != nullptr) {
      insert_repr(new_bucket);
      update_after_insertion();
    }
  }

  // Removes the key from data structure. This key must exist.
  void remove(uint64_t key) {
    --m_size;
    yfast_bucket* p_bucket = pred_bucket(key);
    // Bucket returns which representants have to be changed.
    xfast_update update = p_bucket->remove(key);
    // Here we update the xfast_trie.

    if(update.repr_to_remove != 0) {
      remove_repr(update.repr_to_remove);
      update_after_deletion();
    }
    if(update.repr_to_insert != nullptr) {
      insert_repr(update.repr_to_insert);
      update_after_insertion();
    }
    if(update.bucket_to_delete != nullptr) {
      delete update.bucket_to_delete;
    }
  }

  // Return the predecessor of key. If there are no keys {false, 0} is returned.
  // If there is no predecessor {false, 1} is returned.
  KeyResult<uint64_t> predecessor(uint64_t key) const {
    // Search the bucket that must contain the predecessor.
    yfast_bucket const* const search_bucket = pred_bucket(key);
    // Search for the predecessor in the bucket and return it.
    return search_bucket->predecessor(key);
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