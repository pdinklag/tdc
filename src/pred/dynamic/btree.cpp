#include <tdc/pred/dynamic/btree.hpp>
#include <tdc/pred/dynamic/btree/dynamic_fusion_node.hpp>
#include <tdc/pred/dynamic/btree/sorted_array_node.hpp>

using namespace tdc::pred::dynamic;

// instances
class BTree<uint64_t, 9, DynamicFusionNode<uint64_t, 8>>;
class BTree<uint64_t, 9, SortedArrayNode<uint64_t, 8>>;
class BTree<uint64_t, 65, SortedArrayNode<uint64_t, 64>>;
