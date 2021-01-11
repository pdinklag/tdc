#include <numeric>

#include <tdc/vec/fixed_width_int_vector.hpp>
#include <tdc/test/assert.hpp>

template<size_t bits>
void test_fixed_width_builder() {
    auto vec = typename tdc::vec::FixedWidthIntVector<bits>::builder_type(2);
    ASSERT_EQ(vec.size(), 0);
    ASSERT_EQ(vec.capacity(), 2);
    
    vec.push_back(147);
    vec.push_back(666);
    vec.push_back(1337);
    vec.push_back(65535);
    
    ASSERT_EQ(vec.size(), 4);
    ASSERT_EQ(vec.capacity(), 4);
    ASSERT_EQ(vec[0], 147);
    ASSERT_EQ(vec[1], 666);
    ASSERT_EQ(vec[2], 1337);
    ASSERT_EQ(vec[3], 65535);
    
    ASSERT_EQ(vec.front(), 147);
    ASSERT_EQ(vec.back(), 65535);
    
    ASSERT_EQ(std::accumulate(vec.begin(), vec.end(), 0), 67685);
    
    vec[1] = (uint64_t)vec.back();
    ASSERT_EQ(vec[0], 147);
    ASSERT_EQ(vec[1], 65535);
    ASSERT_EQ(vec[2], 1337);
    ASSERT_EQ(vec[3], 65535);
    
    vec.pop_back();
    ASSERT_EQ(vec[1], 65535);
    ASSERT_EQ(vec.size(), 3);
    ASSERT_EQ(vec.capacity(), 4);
    
    vec.shrink_to_fit();
    ASSERT_EQ(vec.size(), 3);
    ASSERT_EQ(vec.capacity(), 3);
}

int main(int argc, char** argv) {
    test_fixed_width_builder<16>();
}
