#include <numeric>

#include <tdc/util/min_count.hpp>
#include <tdc/test/assert.hpp>

int main(int argc, char** argv) {
    // initialize
    tdc::MinCount<char> ds;
    ASSERT_TRUE(ds.empty());
    
    // insert an item and extract it again
    {
        auto a = ds.insert('A');
        ASSERT_FALSE(ds.empty());
        ASSERT_EQ(ds.min(), 1);
    
        auto min = ds.extract_min();
        ASSERT_EQ(min, 'A');
    }
    ASSERT_TRUE(ds.empty());
    
    // insert an item and increment it
    {
        auto a = ds.insert('A');
        ASSERT_FALSE(ds.empty());
        
        ds.increment(a); // 2
        ASSERT_EQ(a.count(), 2);
        ASSERT_FALSE(ds.empty());
        ASSERT_EQ(ds.min(), 2);
        
        ds.increment(a); // 3
        ASSERT_EQ(a.count(), 3);
        ASSERT_FALSE(ds.empty());
        ASSERT_EQ(ds.min(), 3);
        
        ds.increment(a); // 4
        ASSERT_EQ(a.count(), 4);
        ASSERT_FALSE(ds.empty());
        ASSERT_EQ(ds.min(), 4);
        
        auto min = ds.extract_min();
        ASSERT_EQ(min, 'A');
    }
    ASSERT_TRUE(ds.empty());
    
    // insert multiple items and verify min
    {
        auto a = ds.insert('A');
        auto b = ds.insert('B');
        auto c = ds.insert('C');
        
        {
            auto d = ds.insert('D');
            
            ASSERT_EQ(a.count(), 1);
            ASSERT_EQ(b.count(), 1);
            ASSERT_EQ(c.count(), 1);
            ASSERT_EQ(d.count(), 1);
            ASSERT_EQ(ds.min(), 1);
            
            ds.increment(a); // A -> 2
            ASSERT_EQ(a.count(), 2);
            ASSERT_EQ(b.count(), 1);
            ASSERT_EQ(c.count(), 1);
            ASSERT_EQ(d.count(), 1);
            ASSERT_EQ(ds.min(), 1);
            
            ds.increment(a); // A -> 3
            ASSERT_EQ(a.count(), 3);
            ASSERT_EQ(b.count(), 1);
            ASSERT_EQ(c.count(), 1);
            ASSERT_EQ(d.count(), 1);
            ASSERT_EQ(ds.min(), 1);
            
            ds.increment(b); // B -> 2
            ds.increment(b); // B -> 3
            ASSERT_EQ(a.count(), 3);
            ASSERT_EQ(b.count(), 3);
            ASSERT_EQ(c.count(), 1);
            ASSERT_EQ(d.count(), 1);
            ASSERT_EQ(ds.min(), 1);
            
            ds.increment(c); // C -> 2
            ds.increment(c); // C -> 3

            ASSERT_EQ(a.count(), 3);
            ASSERT_EQ(b.count(), 3);
            ASSERT_EQ(c.count(), 3);
            ASSERT_EQ(d.count(), 1);
            ASSERT_EQ(ds.min(), 1);
            // there should be no bucket with 2 now

            auto min = ds.extract_min();
            ASSERT_EQ(min, 'D');
            
            ASSERT_EQ(ds.min(), 3);
        }
        
        // finally, insert items with starting count
        auto e = ds.insert('E', 2);        
        ASSERT_EQ(e.count(), 2);
        ASSERT_EQ(ds.min(), 2);
        
        auto f = ds.insert('F', 4);  
        ASSERT_EQ(f.count(), 4);
        ASSERT_EQ(ds.min(), 2);
    }
}
