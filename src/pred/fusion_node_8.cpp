#include <iostream> // FIXME: DEBUG

#include <tdc/pred/fusion_node_internals.hpp>
#include <tdc/pred/fusion_node_8.hpp>

using namespace tdc::pred;

FusionNode8::FusionNode8() : m_mask(0) {
}

FusionNode8::FusionNode8(const uint64_t* keys, const size_t num) {
    auto fnode8 = internal::construct(keys, num);
    m_mask = fnode8.mask;
    m_branch = fnode8.branch;
    m_free = fnode8.free;
}

FusionNode8::FusionNode8(const SkipAccessor<uint64_t>& keys, const size_t num) {
    auto fnode8 = internal::construct(keys, num);
    m_mask = fnode8.mask;
    m_branch = fnode8.branch;
    m_free = fnode8.free;
}

Result FusionNode8::predecessor(const uint64_t* keys, const uint64_t x) const {
    return internal::predecessor(keys, x, { m_mask, m_branch, m_free });
}

Result FusionNode8::predecessor(const SkipAccessor<uint64_t>& keys, const uint64_t x) const {
    return internal::predecessor(keys, x, { m_mask, m_branch, m_free });
}
