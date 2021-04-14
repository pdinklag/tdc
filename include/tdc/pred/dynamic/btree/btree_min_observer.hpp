#pragma once

#include <limits>

namespace tdc {
namespace pred {
namespace dynamic {

/// \brief Keeps track of the minimum key contained in a B-Tree.
/// \tparam btree_t the B-Tree type
template<typename btree_t>
requires (btree_t::node_impl_t::is_ordered())
class BTreeMinObserver : public btree_t::Observer {
    public:
        using btree_key_t = btree_t::Observer::btree_key_t;
        using btree_node_t = btree_t::Observer::btree_node_t;
    
    private:
        static constexpr btree_key_t m_key_max = std::numeric_limits<btree_key_t>::max();
        
        const btree_t* m_btree;
        btree_key_t m_min;
    
    public:
        BTreeMinObserver(const btree_t& btree) : m_btree(&btree), m_min(m_key_max) {
        }
    
        virtual void key_inserted(const btree_key_t& key, const btree_node_t& node) override {
            if(key < m_min) {
                // we have a new minimum
                m_min = key;
            }
        }

        virtual void key_removed(const btree_key_t& key, const btree_node_t& node) override {
            if(key == m_min) {
                if(m_btree->size() == 0) {
                    // there is no minimum
                    m_min = m_key_max;
                } else {
                    // get new minimum
                    // it must be contained in the node it was removed from, and there it is precisely the first key
                    m_min = node.impl()[0];
                }
            }
        }
        
        /// \brief Reports the current minimum key contained in the observed B-Tree.
        ///
        /// Only valid if the B-Tree is non-empty.
        btree_key_t min() const {
            return m_min;
        }
};

}}} // namespace tdc::pred::dynamic
