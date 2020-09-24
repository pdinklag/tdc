#include <iostream> // FIXME: DEBUG

#include <tdc/pred/fusion_node_8.hpp>

using namespace tdc::pred;

FusionNode8::FusionNode8() : m_mask(0) {
}

FusionNode8::FusionNode8(const uint64_t* keys, const size_t num) {
    auto fnode8 = Internals::construct(keys, num);
    m_mask = std::get<0>(fnode8);
    m_branch = std::get<1>(fnode8);
    m_free = std::get<2>(fnode8);
}

FusionNode8::FusionNode8(const SkipAccessor<uint64_t>& keys, const size_t num) {
    auto fnode8 = Internals::construct(keys, num);
    m_mask = std::get<0>(fnode8);
    m_branch = std::get<1>(fnode8);
    m_free = std::get<2>(fnode8);
}

Result FusionNode8::predecessor(const uint64_t* keys, const uint64_t x) const {
    return Internals::predecessor(keys, x, m_mask, m_branch, m_free);
}

Result FusionNode8::predecessor(const SkipAccessor<uint64_t>& keys, const uint64_t x) const {
    return Internals::predecessor(keys, x, m_mask, m_branch, m_free);
}
