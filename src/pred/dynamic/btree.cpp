#include <tdc/pred/dynamic/btree.hpp>
#include <tdc/pred/dynamic/btree/dynamic_fusion_node.hpp>
#include <tdc/pred/dynamic/btree/sorted_array_node.hpp>

using namespace tdc::pred::dynamic;

// instances
class BTree<DynamicFusionNode<>, 9>;
class BTree<SortedArrayNode<8>, 9>;
