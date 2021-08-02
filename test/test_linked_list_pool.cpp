#include <tdc/util/linked_list_pool.hpp>

#include <tdc/test/assert.hpp>

void basic_test(tdc::LinkedListPool<int>& pool) {
    auto list = pool.new_list();
    
    // sanity
    //~ ASSERT_EQ(list.size(), 0);
    ASSERT_TRUE(list.empty());

    // insert
    for(int i = 0; i < 10; i++) {
        list.emplace_front(i);
        list.verify();

        ASSERT_FALSE(list.empty());
        //~ ASSERT_EQ(list.size(), (size_t)(i+1));
        ASSERT_EQ(*list.begin(), i);
    }

    // iterate
    {
        int i = 9;
        auto it = list.begin();
        while(it != list.end()) {
            ASSERT_EQ(*it, i);

            ++it;
            --i;
        }
        ASSERT_EQ(i, -1);
    }

    // erase
    {
        int i = 9;
        while(!list.empty()) {
            auto it = list.begin();
            ASSERT_EQ(*it, i);

            list.erase(it);
            list.verify();
            
            //~ ASSERT_EQ(list.size(), i);
            --i;
        }            
        ASSERT_EQ(i, -1);
    }
}

void ref_test(tdc::LinkedListPool<int>& pool) {
    auto list = pool.new_list();
    tdc::LinkedListPool<int>::Iterator refs[10];
    
    // insert and save refs
    for(int i = 0; i < 10; i++) {
        list.emplace_front(i);
        list.verify();
        refs[i] = list.begin();
    }

    // verify refs
    for(int i = 0; i < 10; i++) {
        ASSERT_EQ(*refs[i], i);
    }

    // do some stuff in basic test (creates another temporary list)
    basic_test(pool);

    // verify refs
    for(int i = 0; i < 10; i++) {
        ASSERT_EQ(*refs[i], i);
    }

    // erase in insertion order
    for(int i = 0; i < 10; i++) {
        list.erase(refs[i]);
        list.verify();
        //~ ASSERT_EQ(list.size(), 9-i);
    }

    ASSERT_TRUE(list.empty());
}

int main(int argc, char** argv) {
    tdc::LinkedListPool<int> pool;
    basic_test(pool);
    ref_test(pool);
}
