#include <iostream> // FIXME: DEBUG

#include <tdc/pred/fusion_node.hpp>

using namespace tdc::pred;

FusionNode::FusionNode() : m_mask(0) {
}

FusionNode::FusionNode(const uint64_t* keys, const size_t num) {
    auto fnode8 = Internals::construct(keys, num);
    m_mask = std::get<0>(fnode8);
    m_branch = std::get<1>(fnode8);
    m_free = std::get<2>(fnode8);
}

FusionNode::FusionNode(const SkipAccessor<uint64_t>& keys, const size_t num) {
    auto fnode8 = Internals::construct(keys, num);
    m_mask = std::get<0>(fnode8);
    m_branch = std::get<1>(fnode8);
    m_free = std::get<2>(fnode8);
}

Result FusionNode::predecessor(const uint64_t* keys, const uint64_t x) const {
    return Internals::predecessor(keys, x, m_mask, m_branch, m_free);
}

Result FusionNode::predecessor(const SkipAccessor<uint64_t>& keys, const uint64_t x) const {
    return Internals::predecessor(keys, x, m_mask, m_branch, m_free);
}
