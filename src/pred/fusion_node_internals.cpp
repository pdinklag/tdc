#include <tdc/pred/fusion_node_internals.hpp>

class tdc::pred::internal::FusionNodeInternals<uint64_t, 8, false>;
class tdc::pred::internal::FusionNodeInternals<uint64_t, 16, false>;
class tdc::pred::internal::FusionNodeInternals<uint64_t, 32, false>;

class tdc::pred::internal::FusionNodeInternals<uint64_t, 8, true>;
class tdc::pred::internal::FusionNodeInternals<uint64_t, 16, true>;
class tdc::pred::internal::FusionNodeInternals<uint64_t, 32, true>;
