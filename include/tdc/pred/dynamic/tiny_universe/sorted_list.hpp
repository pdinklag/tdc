#pragma once

#include <algorithm>
#include <tdc/pred/result.hpp>

namespace tdc {
namespace pred {
namespace dynamic {

template <typename t_value_type>
class SortedList {
  std::vector<t_value_type> m_elem;

 public:
  inline size_t size() const {
    return m_elem.size();
  }

  inline void insert(t_value_type key) {
    m_elem.push_back(0);
    size_t i = m_elem.size()-1;
    while ((i != 0) && (m_elem[i - 1] >= key)) {
      m_elem[i] = m_elem[i - 1];
      --i;
    }
    m_elem[i] = key;
  }

  inline void remove(t_value_type key) {
    for(auto it = std::lower_bound(m_elem.begin(), m_elem.end(), key); it != m_elem.end() - 1; ++it) {
        *it = *(it+1);
    }
    m_elem.pop_back();
  }

  inline KeyResult<t_value_type> predecessor(t_value_type key) const {
    auto it = std::upper_bound(m_elem.begin(), m_elem.end(), key);
    return pred::KeyResult<t_value_type> { it != m_elem.begin(), *(--it) };
  }
};

}  // namespace dynamic
}  // namespace pred
}  // namespace tdc