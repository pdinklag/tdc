#include <numeric>

#include <tdc/pred/dynamic/btree.hpp>
#include <tdc/pred/dynamic/btree/btree_min_observer.hpp>
#include <tdc/pred/dynamic/btree/sorted_array_node.hpp>
#include <tdc/test/assert.hpp>

using Key = uint64_t;
using BTree = tdc::pred::dynamic::BTree<Key, 65, tdc::pred::dynamic::SortedArrayNode<Key, 64>>;

int main(int argc, char** argv) {
    BTree btree;
    tdc::pred::dynamic::BTreeMinObserver obs(btree);
    btree.set_observer(&obs);
    
    // insert
    for(uint64_t x = 100; x > 0; x--) {
        btree.insert(x);
        ASSERT_EQ(obs.min(), x);
    }
    
    btree.insert(101);
    ASSERT_EQ(obs.min(), 1);
    
    // remove
    btree.remove(101);
    ASSERT_EQ(obs.min(), 1);
    
    for(uint64_t x = 1; x < 100; x++) {
        btree.remove(x);
        ASSERT_EQ(obs.min(), x+1);
    }
}
