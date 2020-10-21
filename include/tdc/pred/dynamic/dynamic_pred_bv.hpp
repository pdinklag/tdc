#include <assert.h>

#include <cstdint>
#include <limits>
#include <tdc/pred/result.hpp>
#include <tdc/util/likely.hpp>
#include <vector>

namespace tdc {
namespace pred {
namespace dynamic {

class DynPredBV {
 private:
  std::vector<bool> m_bv;
  uint64_t m_min = std::numeric_limits<uint64_t>::max();  // stores the minimal key
  uint64_t m_max = std::numeric_limits<uint64_t>::min();  // stores the maximal key
  size_t m_size = 0;

 public:
  DynPredBV() = default;
  DynPredBV(const uint64_t* keys, const size_t num) {
    for (size_t i = 0; i < num; ++i) {
      insert(keys[i]);
    }
  }

  size_t size() const {
    return m_size;
  }

  void insert(uint64_t key) {
    if (key >= m_bv.size()) {
      m_bv.resize(key + 1, false);
      m_max = std::max(m_max, key);
    }
    assert(!m_bv[key]);
    m_bv[key] = true;
    m_min = std::min(m_min, key);
    ++m_size;
  }

  void remove(uint64_t key) {
    assert(m_bv[key]);
    m_bv[key] = false;
    --m_size;

    if (m_size == 0) {
      std::vector<bool>().swap(m_bv);
      m_min = std::numeric_limits<uint64_t>::max();
      m_max = std::numeric_limits<uint64_t>::min();
      return;
    }

    if (key == m_max) {
      m_max = predecessor(key);
      auto pred = predecessor(key);
      assert(pred.exists);
      m_bv.resize(pred.key);
      m_bv.shrink_to_fit();
    }

    if (key == m_min) {
      m_min = successor(key);
    }
  }

  KeyResult<uint64_t> predecessor(uint64_t key) const {
    if (tdc_unlikely(m_size == 0)) {
      return {false, 0};
    }
    if (tdc_unlikely(key < m_min)) {
      return {false, 1};
    }
    if (tdc_unlikely(key >= m_max)) {
      return {true, m_max};
    }

    while (!m_bv[key]) {
      --key;
    }
    return {true, key};
  }

  KeyResult<uint64_t> successor(uint64_t key) const {
    if (tdc_unlikely(m_size == 0)) {
      return {false, 0};
    }
    if (tdc_unlikely(key > m_max)) {
      return {false, 1};
    }
    if (tdc_unlikely(key <= m_min)) {
      return {true, m_min};
    }
    while (!m_bv[key]) {
      ++key;
    }
    return {true, key};
  }
};
}  // namespace dynamic
}  // namespace pred
}  // namespace tdc
