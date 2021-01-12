#pragma once

#include <tdc/pred/result.hpp>

namespace tdc {
namespace pred {
namespace dynamic {

template <typename t_value_type>
class UnsortedList {
  std::vector<t_value_type> m_elem;

 public:
  inline size_t size() const {
    return m_elem.size();
  }
  inline void insert(t_value_type key) {
    m_elem.push_back(key);
  }

  inline void remove(t_value_type key) {
    auto del = std::find(m_elem.begin(), m_elem.end(), key);
    assert(del != m_elem.end());
    *del = m_elem.back();
    m_elem.pop_back();
  }

  inline KeyResult<t_value_type> predecessor(t_value_type key) const {
    bool found = false;
    t_value_type max_pred = 0;
    for (auto elem : m_elem) {
      found = found || (elem <= key);
      max_pred = std::max(elem, max_pred);
    } 
    return {found, max_pred};
  }
};  // namespace dynamic

}  // namespace dynamic
}  // namespace pred
}  // namespace tdc