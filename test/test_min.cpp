#include <numeric>

#include <tdc/util/min_count.hpp>
#include <tdc/util/min_count_map.hpp>
#include <tdc/test/assert.hpp>

void test_min() {
    // initialize
    tdc::MinCount<char> ds;
    ASSERT_TRUE(ds.empty());
    ASSERT_EQ(ds.num_buckets(), 0);
    
    // insert an item and extract it again
    {
        auto a = ds.insert('A');
        ASSERT_FALSE(ds.empty());
        ASSERT_EQ(ds.min(), 1);
        ASSERT_EQ(ds.num_buckets(), 1);
    
        auto min = ds.extract_min();
        ASSERT_EQ(min, 'A');
        ASSERT_EQ(ds.num_buckets(), 0);
    }
    ASSERT_TRUE(ds.empty());
    
    // insert an item and increment it
    {
        auto a = ds.insert('A');
        ASSERT_FALSE(ds.empty());
        ASSERT_EQ(ds.num_buckets(), 1);
        
        ds.increment(a); // 2
        ASSERT_EQ(a.count(), 2);
        ASSERT_FALSE(ds.empty());
        ASSERT_EQ(ds.num_buckets(), 1);
        ASSERT_EQ(ds.min(), 2);
        
        ds.increment(a); // 3
        ASSERT_EQ(a.count(), 3);
        ASSERT_FALSE(ds.empty());
        ASSERT_EQ(ds.num_buckets(), 1);
        ASSERT_EQ(ds.min(), 3);
        
        ds.increment(a); // 4
        ASSERT_EQ(a.count(), 4);
        ASSERT_FALSE(ds.empty());
        ASSERT_EQ(ds.num_buckets(), 1);
        ASSERT_EQ(ds.min(), 4);
        
        auto min = ds.extract_min();
        ASSERT_EQ(min, 'A');        
        ASSERT_EQ(ds.num_buckets(), 0);
    }
    ASSERT_TRUE(ds.empty());
    
    // insert multiple items and verify min
    {
        auto a = ds.insert('A');
        auto b = ds.insert('B');
        auto c = ds.insert('C');
        ASSERT_EQ(ds.num_buckets(), 1);
        
        {
            auto d = ds.insert('D');
            ASSERT_EQ(ds.num_buckets(), 1);
            
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
                
            ASSERT_EQ(ds.num_buckets(), 2); // just 1 (D) and 3 (A, B, C)
            
            auto min = ds.extract_min();
            ASSERT_EQ(min, 'D');
            
            ASSERT_EQ(ds.min(), 3);
            ASSERT_EQ(ds.num_buckets(), 1); // just 3 (A, B, C)
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

void test_min_map() {
    // initialize
    tdc::MinCountMap<char> ds;
    
    ASSERT_TRUE(ds.empty());
    ASSERT_EQ(ds.num_buckets(), 0);
    
    // insert an item and extract it again
    {
        auto a = ds.insert('A');
        ASSERT_FALSE(ds.empty());
        ASSERT_EQ(ds.min(), a);
        ASSERT_EQ(ds.num_buckets(), 1);
    
        auto min = ds.extract_min();
        ASSERT_EQ(min, a);
        ASSERT_EQ(ds.num_buckets(), 0);
    }
    ASSERT_TRUE(ds.empty());
    
    // insert an item and increment it
    {
        auto a = ds.insert('A');
        ASSERT_FALSE(ds.empty());
        ASSERT_EQ(ds.num_buckets(), 1);
        
        a = ds.increment(a); // 2
        ASSERT_EQ(a.count, 2);
        ASSERT_FALSE(ds.empty());
        ASSERT_EQ(ds.num_buckets(), 1);
        ASSERT_EQ(ds.min(), a);
        
        a = ds.increment(a); // 3
        ASSERT_EQ(a.count, 3);
        ASSERT_FALSE(ds.empty());
        ASSERT_EQ(ds.num_buckets(), 1);
        ASSERT_EQ(ds.min(), a);
        
        a = ds.increment(a); // 4
        ASSERT_EQ(a.count, 4);
        ASSERT_FALSE(ds.empty());
        ASSERT_EQ(ds.num_buckets(), 1);
        ASSERT_EQ(ds.min(), a);
        
        auto min = ds.extract_min();
        ASSERT_EQ(min, a);
        ASSERT_EQ(ds.num_buckets(), 0);
    }
    ASSERT_TRUE(ds.empty());
    
    // insert multiple items and verify min
    {
        auto a = ds.insert('A');
        auto b = ds.insert('B');
        auto c = ds.insert('C');
        ASSERT_EQ(ds.num_buckets(), 1);
        
        {
            auto d = ds.insert('D');
            ASSERT_EQ(ds.num_buckets(), 1);
            
            ASSERT_EQ(a.count, 1);
            ASSERT_EQ(b.count, 1);
            ASSERT_EQ(c.count, 1);
            ASSERT_EQ(d.count, 1);
            ASSERT_EQ(ds.min().count, 1);
            
            a = ds.increment(a); // A -> 2
            ASSERT_EQ(a.count, 2);
            ASSERT_EQ(b.count, 1);
            ASSERT_EQ(c.count, 1);
            ASSERT_EQ(d.count, 1);
            ASSERT_EQ(ds.min().count, 1);
            ASSERT_EQ(ds.num_buckets(), 2); // 1 (B, C, D) and 2 (A)
            
            a = ds.increment(a); // A -> 3
            ASSERT_EQ(a.count, 3);
            ASSERT_EQ(b.count, 1);
            ASSERT_EQ(c.count, 1);
            ASSERT_EQ(d.count, 1);
            ASSERT_EQ(ds.min().count, 1);
            ASSERT_EQ(ds.num_buckets(), 2); // 1 (B, C, D) and 3 (A)
            
            b = ds.increment(b); // B -> 2
            ASSERT_EQ(ds.num_buckets(), 3); // 1 (C, D), 2 (B) and 3 (A)
            b = ds.increment(b); // B -> 3
            ASSERT_EQ(a.count, 3);
            ASSERT_EQ(b.count, 3);
            ASSERT_EQ(c.count, 1);
            ASSERT_EQ(d.count, 1);
            ASSERT_EQ(ds.min().count, 1);
            ASSERT_EQ(ds.num_buckets(), 2); // 1 (C, D) and 3 (A, B)
            
            c = ds.increment(c); // C -> 2
            c = ds.increment(c); // C -> 3

            ASSERT_EQ(a.count, 3);
            ASSERT_EQ(b.count, 3);
            ASSERT_EQ(c.count, 3);
            ASSERT_EQ(d.count, 1);
            ASSERT_EQ(ds.min().count, 1);
            ASSERT_EQ(ds.num_buckets(), 2); // just 1 (D) and 3 (A, B, C)
            
            auto min = ds.extract_min();
            ASSERT_EQ(min, d);
            
            ASSERT_EQ(ds.min().count, 3);
            ASSERT_EQ(ds.num_buckets(), 1); // just 3 (A, B, C)
        }
        
        // finally, insert items with starting count
        auto e = ds.insert('E', 2);        
        ASSERT_EQ(e.count, 2);
        ASSERT_EQ(ds.min().count, 2);
        
        auto f = ds.insert('F', 4);  
        ASSERT_EQ(f.count, 4);
        ASSERT_EQ(ds.min().count, 2);
    }
}

int main(int argc, char** argv) {
    test_min();
    test_min_map();
}
