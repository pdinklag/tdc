#include <cstdlib>
#include <iostream>

#define ASSERT_FAIL_STR "Assertion failed: "

#define ASSERT_TRUE(x) \
{ \
    if(!x) { \
        std::cerr \
            << ASSERT_FAIL_STR \
            << __FILE__ << ":" << __LINE__ \
            << " in " << __PRETTY_FUNCTION__ \
            << std::endl; \
        std::flush(std::cerr); \
        std::abort(); \
    } \
}

#define ASSERT_FALSE(x) \
    ASSERT_TRUE((!x))

#define ASSERT_EQ(x, y) \
    ASSERT_TRUE((x == y))

#define ASSERT_NEQ(x, y) \
    ASSERT_TRUE((x != y))

#define ASSERT_GT(x, y) \
    ASSERT_TRUE((x > y))

#define ASSERT_GEQ(x, y) \
    ASSERT_TRUE((x >= y))

#define ASSERT_LT(x, y) \
    ASSERT_TRUE((x < y))

#define ASSERT_LEQ(x, y) \
    ASSERT_TRUE((x <= y))
