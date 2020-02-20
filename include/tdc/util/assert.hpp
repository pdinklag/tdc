#pragma once

#include <cassert>

namespace tdc {

/// \brief Asserts that the items in the array in question are in ascending order.
/// \tparam array_t the array type, must support the <tt>[]</tt> operator and items must support the <tt><=</tt> operator
/// \param a the array in question
/// \param size the number of items in the array
template<typename array_t>
inline void assert_sorted_ascending(const array_t& a, const size_t size) {
    #ifndef NDEBUG
    if(size > 1) {                
        auto prev = a[0];
        for(size_t i = 1; i < size; i++) {
            auto next = a[i];
            assert(prev <= next);
            prev = next;
        }
    }
    #endif
}

}
