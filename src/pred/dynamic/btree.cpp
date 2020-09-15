#include <tdc/pred/dynamic/btree.hpp>
#include <tdc/pred/dynamic/btree/dynamic_fusion_node_8.hpp>
#include <tdc/pred/dynamic/btree/sorted_array_node_8.hpp>

using namespace tdc::pred::dynamic;

// instances
class BTree<DynamicFusionNode8, 9>;
class BTree<SortedArrayNode8, 9>;
