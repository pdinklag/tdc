#pragma once

#include <cassert>
#include <limits>

namespace tdc {
namespace pred {
namespace dynamic {

/// \brief Keeps track of the minimum key contained in a B-Tree.
///
/// Note that the tracked minimum is only the minimum after the beginning of the observation.
/// For correct results, the observer should be registered right after constructing the B-Tree.
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
        bool m_has_min;
    
    public:
        BTreeMinObserver() : m_btree(nullptr), m_has_min(false) {
        }
    
        BTreeMinObserver(const btree_t& btree) : m_btree(&btree), m_has_min(false) {
            assert(btree.size() == 0); // B-Tree must be empty when the observation begins
        }
        
        BTreeMinObserver(const BTreeMinObserver&) = default;
        BTreeMinObserver(BTreeMinObserver&&) = default;
        BTreeMinObserver& operator=(const BTreeMinObserver&) = default;
        BTreeMinObserver& operator=(BTreeMinObserver&&) = default;
    
        virtual void key_inserted(const btree_key_t& key, const btree_node_t& node) override {
            if(!m_has_min || key < m_min) {
                // we have a new minimum
                m_min = key;
                m_has_min = true;
            }
        }

        virtual void key_removed(const btree_key_t& key, const btree_node_t& node) override {
            assert(m_has_min);
            if(key == m_min) {
                if(m_btree->size() == 0) {
                    // there is no minimum
                    m_has_min = false;
                } else {
                    // get new minimum
                    // it must be contained in the node it was removed from, and there it is precisely the first key
                    m_min = node.impl()[0];
                }
            }
        }
        
        /// \brief Reports whether a minimum is currently available.
        ///
        /// This is the case as soon as any insertion has been observed.
        bool has_min() const {
            return m_has_min;
        }
        
        /// \brief Reports the current minimum key contained in the observed B-Tree.
        ///
        /// Only valid if the B-Tree is non-empty.
        btree_key_t min() const {
            return m_min;
        }
};

}}} // namespace tdc::pred::dynamic
