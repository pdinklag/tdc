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
};

template <uint8_t t_bucket_width>
struct yfast_bucket {
  static constexpr size_t BUCKET_SIZE = 1ULL << t_bucket_width;
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

  uint64_t get_repr() { return m_min; }

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
    for (size_t i = 0; i < size(); ++i) {
      //std::cout << m_elem[i] << " ";
    }
    //std::cout << std::endl;
    if (m_elem.size() >= 2 * BUCKET_SIZE) {
      update.insert_repr.push_back(split());
    }
    return update;
  }

  //splits the 2*BUCKET_SIZE big bucket into two equal sized ones
  //and returns the pointer to the new one, so it can be inserted
  //into the xfast trie
  yfast_bucket* split() {
    std::sort(m_elem.begin(), m_elem.end());

    // construct next bucket and adjust pointers
    yfast_bucket* next_b = new yfast_bucket(m_elem[BUCKET_SIZE], this, m_next);
    if (m_next != nullptr) {
      m_next->m_prev = next_b;
    }
    m_next = next_b;

    // move elements bigger than the median into the new bucket
    for (size_t i = BUCKET_SIZE + 1; i < BUCKET_SIZE * 2; ++i) {
      next_b->insert(m_elem[i]);
    }
    m_elem.resize(BUCKET_SIZE);

    // return new bucket
    return next_b;
  }

  xfast_update remove(const uint64_t x) {
    xfast_update update;
    // if it is the last element
    if (size() == 0) {
      if (m_prev != nullptr) {
        m_prev->m_next = m_next;
      }
      if (m_next != nullptr) {
        m_next->m_prev = m_prev;
      }
      update.remove_repr.push_back(x);
      return update;
    }
    if (x == m_min) {
      update.remove_repr.push_back(m_min);
      auto new_min = std::min_element(m_elem.begin(), m_elem.end());
      m_min = *new_min;
      *new_min = m_elem.back();
      m_elem.pop_back();
      update.insert_repr.push_back(this);
    } else {
      auto del = std::find(m_elem.begin(), m_elem.end(), x);
      *del = m_elem.back();
      m_elem.pop_back();
    }

    // if(m_elem.size() == BUCKET_SIZE/4) {
    //   update.remove_repr(merge());
    // }
    return update;
    //   // remove every item from this bucket
    //   for(size_t i = 0; i < m_elem.size(); ++i) {
    //     //...
    //   }
  }

  //returns predecessor in bucket. 
  //it is required that m_min <= key  
  KeyResult<uint64_t> predecessor(uint64_t key) const {
    if(key < m_min) {
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

template <template <uint8_t> class t_bucket, uint8_t t_key_width, uint8_t t_bucket_width>
class YFastTrie {
 private:
  using bucket = t_bucket<t_bucket_width>;
  size_t m_size = 0;

  std::array<std::unordered_map<uint64_t, bucket*>, t_key_width + 1> m_xfast;

  void insert_repr(uint64_t key, bucket* b) {
    m_xfast[0][key] = b;

    uint64_t level = 1;
    key >>= 1;
    // we first find zeroes
    while (level <= t_key_width && (m_xfast[level].find(key) == m_xfast[level].end())) {
      m_xfast[level][key] = b;
      ++level;
      key >>= 1;
    }

    // then we find ones; if one children is a zero, we update the pointer
    while (level <= t_key_width) {
      auto l_child = m_xfast[level - 1].find((key << 1) + 0);
      if (l_child == m_xfast[level - 1].end()) {
        if (m_xfast[level][key]->get_repr() > b->get_repr()) {
          m_xfast[level][key] = b;
        }
        ++level;
        key >>= 1;
        continue;
      }

      auto r_child = m_xfast[level - 1].find((key << 1) + 1);
      if (r_child == m_xfast[level - 1].end()) {
        if (m_xfast[level][key]->get_repr() < b->get_repr()) {
          m_xfast[level][key] = b;
        }
        ++level;
        key >>= 1;
        continue;
      }
      // if both children are ones, we remove the pointer
      m_xfast[level][key] = nullptr;
      ++level;
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
  // TODO: speedup by jumping
  bucket* min_repr(uint64_t level, uint64_t key) {
    while (level > 0) {
      --level;
      key <<= 1;
      if (m_xfast[level].find(key) == m_xfast[level].end()) {
        ++key;
      }
    }
    return m_xfast[0].at(key);
  }
  // returns the bucket with the biggest representant in the subtree from the node
  // that corrospondents to (level, key)
  // TODO: speedup by jumping
  bucket* max_repr(uint64_t level, uint64_t key) {
    while (level > 0) {
      --level;
      key <<= 1;
      ++key;
      if (m_xfast[level].find(key) == m_xfast[level].end()) {
        --key;
      }
    }
    return m_xfast[0].at(key);
  }

 public:
  YFastTrie(){};
  YFastTrie(const uint64_t* keys, const size_t num) {
    // std::fill(m_xfast.begin(), m_xfast.end(), nullptr);
    for (size_t i = 0; i < num; ++i) {
      insert(keys[i]);
    }
  }

  ~YFastTrie() {
    if(m_size != 0) {
      //get bucket with maximal representative and delete every bucket from last to first
      bucket* b = max_repr(t_key_width, 0);
      bucket* p;  
      while(b != nullptr) {
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
    if (m_size == 1) {
      bucket* new_b = new bucket(key, nullptr, nullptr);
      insert_repr(key, new_b);
      return;
    }

    // search for the first 1,
    xfast_update xfu;
    uint64_t prefix = key;
    for (size_t i = 0; i <= t_key_width; ++i) {
      auto elem = m_xfast[i].find(prefix);
      if (elem != m_xfast[i].end()) {
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
    for (auto new_bucket : xfu.insert_repr) {
      //std::cout << "u" << ((bucket*)new_bucket)->m_min << "\n";
      insert_repr(((bucket*)new_bucket)->m_min, (bucket*)new_bucket);
    }
    for (auto old_repr : xfu.remove_repr) {
      //std::cout << "d" << old_repr << "\n";
      remove_repr(old_repr);
    }
  }

  bucket* pred_bucket(uint64_t key) const {
    /*
    //alternative: linear search
    for (size_t level = 0; level <= t_key_width; ++level) {
      auto b = m_xfast[level].find(key);
      if (b != m_xfast[level].end()) {
        return b->second;
      }

      key >>= 1;
    }
    */
   auto repr_bucket = m_xfast[0].find(key);
    if (repr_bucket != m_xfast[0].end()) {
      return repr_bucket->second;
    }

    size_t l = 0;            //point to highest 0
    size_t r = t_key_width;  //points to lowest 1
    size_t m = l + r / 2;

    while (l + 1 != r) {
      auto mapped = m_xfast[m].find(key >> m);
      if (mapped != m_xfast[m].end()) {
        r = m;
      } else {
        l = m;
      }
      m = (l + r) / 2;
    }
    bucket* b = m_xfast[r].at(key >> r);

    if (((key >> l) & 1) == 1) {
      return b;
    } else {
      return b->m_prev;
    }
  }

  //removes element from bucket. bucket returns which representants have to be changed
  void remove(uint64_t key) {
    --m_size;
    bucket* p_bucket = pred_bucket(key);
    xfast_update xfu = p_bucket->remove(key);
    // update xfast
    for (auto new_bucket : xfu.insert_repr) {
      //std::cout << "u" << ((bucket*)new_bucket)->m_min << "\n";
      insert_repr(((bucket*)new_bucket)->m_min, (bucket*)new_bucket);
    }
    for (auto old_repr : xfu.remove_repr) {
      //std::cout << "d" << old_repr << "\n";
      auto b = m_xfast[0].find(old_repr);
      if(b != m_xfast[0].end() && b->second->m_min == key) {
        delete b->second;
      }
      remove_repr(old_repr);
    }
  }

  KeyResult<uint64_t> predecessor(uint64_t key) const {
    if (m_size == 0) {
      return {false, 0};
    }    
    return pred_bucket(key)->predecessor(key);
  }

  void test(size_t n) {
    std::vector<uint64_t> perm;
    for (size_t i = 0; i < (1ULL << t_key_width); ++i) {
      perm.push_back(i);
    }
    std::random_shuffle(perm.begin(), perm.end());

    std::cout << *this;
    for (size_t i = 0; i < n; ++i) {
      //std::cout << "insert " << perm[i] << '\n';
      insert(perm[i]);
      //std::cout << *this;
      //std::cout << m_xfast[t_key_width][0]->m_min << std::endl;
    }
    std::cout << *this;

    for (size_t i = 0; i < (1ULL << t_key_width); ++i) {
      auto r = predecessor(i);
      std::cout << i << "->" << r.exists << "-" << r.key << " ";
      std::cout << std::endl;
    }

    for (size_t i = 0; i < n; ++i) {
      //std::cout << "delete " << perm[i] << '\n';
      remove(perm[i]);
      //std::cout << *this;
    }
    std::cout << *this;
  }

  friend std::ostream&
  operator<<(std::ostream& o, const YFastTrie& yfast) {
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
