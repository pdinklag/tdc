#ifdef PLADS_FOUND

#include <tdc/pred/dynamic/dynamic_rankselect.hpp>

using namespace tdc::pred::dynamic;

DynamicRankSelect::DynamicRankSelect() : m_size(0) {
}

tdc::pred::Result DynamicRankSelect::predecessor(const uint64_t x) const {
    const uint64_t rank = m_dbv.rank<1>(x);
    return Result { rank > 0, m_dbv.select<1>(rank) };
}

void DynamicRankSelect::insert(const uint64_t key) {
    if(key >= m_dbv.size()) {
        m_dbv.resize(key + 1, 0);
    }
    
    assert(!m_dbv.get(key));
    m_dbv.set(key, 1);
    ++m_size;
}

bool DynamicRankSelect::remove(const uint64_t key) {
    assert(m_size > 0);
    
    const bool b = m_dbv.get(key);
    if(b) {
        m_dbv.set(key, 0);
        --m_size;
    }
    return b;
}

#endif
