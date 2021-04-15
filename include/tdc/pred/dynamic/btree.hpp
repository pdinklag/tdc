#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <memory>
#include <type_traits>

#include <tdc/pred/result.hpp>
#include <tdc/util/concepts.hpp>
#include <tdc/util/likely.hpp>

namespace tdc {
namespace pred {
namespace dynamic {

template<typename T, typename key_t>
concept BTreeNodeImpl = SizedIndexAccessTo<T, key_t> && requires(T node, key_t key) {
    { node.predecessor(key) } -> std::same_as<PosResult>;
    { node.insert(key) };
    { node.remove(key) } -> std::same_as<bool>;
};

/// \brief A B-Tree.
/// \tparam key_t the key type
/// \tparam degree the maximum number of children per node
/// \tparam node_impl_t node implementation; must be sorted and support array access, size, insert, remove and predecessor
template<std::totally_ordered m_key_t, size_t m_degree, BTreeNodeImpl<m_key_t> m_node_impl_t>
class BTree {
public:
    /// \brief The contained key type.
    using key_t = m_key_t;
    
    /// \brief The data structure that manages keys contained in a node.
    using node_impl_t = m_node_impl_t;
    
    /// \brief The maximum number of children of a node.
    ///
    /// The number of keys (splitters) contained in a node equals this value minus one.
    static constexpr size_t degree = m_degree;

    /// \brief A node in the B-Tree.
    class Node;

    /// \brief Base class for B-Tree observers.
    class Observer {
    protected:
        using btree_key_t = key_t;
        using btree_node_t = Node;

    public:
        /// \brief Called when a new key was inserted into the observed B-Tree.
        /// \param key the inserted key
        /// \param node the node into which the key was inserted
        virtual void key_inserted(const btree_key_t& key, const btree_node_t& node) {
        }

        /// \brief Called when a key was removed from the observed B-Tree.
        /// \param key the removed key
        /// \param node the node from which the key was removed; may be \c nullptr in case the node was also removed as a result
        virtual void key_removed(const btree_key_t& key, const btree_node_t& node) {
        }
    };

private:
    static_assert((m_degree % 2) == 1); // we only allow odd maximum degrees for the sake of implementation simplicity
    static_assert(m_degree > 1);
    static_assert(m_degree < 65536);
    
    using childcount_t = typename std::conditional<m_degree < 256, uint8_t, uint16_t>::type;
    
    static constexpr size_t m_max_node_keys = m_degree - 1;
    static constexpr size_t m_split_right = m_max_node_keys / 2;
    static constexpr size_t m_split_mid = m_split_right - 1;
    static constexpr size_t m_deletion_threshold = m_degree / 2;

public:
    class Node {
    private:
        friend class BTree;
    
        node_impl_t m_impl;
        childcount_t m_num_children;
        Node** m_children;
    
    public:
        /// \brief Reports whether this node is a leaf.
        inline bool is_leaf() const {
            return m_children == nullptr;
        }
        
        /// \brief Reports the number of keys (splitters) contained in this node.
        ///
        /// Note that this is not the number of children.
        inline size_t size() const {
            return m_impl.size();
        }

        /// \brief Returns the node implementation that manages the contained keys.
        const node_impl_t& impl() const {
            return m_impl;
        }
        
        /// \brief Reports the number of children of this node.
        childcount_t num_children() const {
            return m_num_children;
        }
        
        /// \brief Retrieves a pointer to the i-th child.
        ///
        /// Only valid if i is less than \ref num_children.
        const Node* child(const size_t i) {
            return m_children[i];
        }
        
    private:
        inline bool is_empty() const {
            return size() == 0;
        }

        inline bool is_full() const {
            return size() == m_max_node_keys;
        }

        Node() : m_children(nullptr), m_num_children(0) {
        }
        
        ~Node() {
            for(size_t i = 0; i < m_num_children; i++) {
                delete m_children[i];
            }
            if(m_children) delete[] m_children;
        }

        Node(const Node&) = default;
        Node(Node&&) = default;
        Node& operator=(const Node&) = default;
        Node& operator=(Node&&) = default;

        inline void alloc_m_children() {
            if(m_children == nullptr) {
                m_children = new Node*[m_degree];
            }
        }

        void insert_child(const size_t i, Node* node) {
            assert(i <= m_num_children);
            assert(m_num_children < m_degree);
            alloc_m_children();
            
            // insert
            for(size_t j = m_num_children; j > i; j--) {
                m_children[j] = m_children[j-1];
            }
            m_children[i] = node;
            ++m_num_children;
        }
        
        void remove_child(const size_t i) {
            assert(m_num_children > 0);
            assert(i <= m_num_children);

            for(size_t j = i; j < m_num_children-1; j++) {
                m_children[j] = m_children[j+1];
            }
            m_children[m_num_children-1] = nullptr;
            --m_num_children;
            
            if(m_num_children == 0) {
                delete[] m_children;
                m_children = nullptr;
            }
        }

        void split_child(const size_t i) {
            assert(!is_full());
            
            Node* y = m_children[i];
            assert(y->is_full());

            // allocate new node
            Node* z = new Node();

            // get the middle value
            const key_t m = y->m_impl[m_split_mid];

            // move the keys larger than middle from y to z and remove the middle
            {
                key_t buf[m_split_right];
                for(size_t j = 0; j < m_split_right; j++) {
                    buf[j] = y->m_impl[j + m_split_right];
                    z->m_impl.insert(buf[j]);
                }
                for(size_t j = 0; j < m_split_right; j++) {
                    y->m_impl.remove(buf[j]);
                }
                y->m_impl.remove(m);
            }

            // move the m_children right of middle from y to z
            if(!y->is_leaf()) {
                z->alloc_m_children();
                for(size_t j = m_split_right; j <= m_max_node_keys; j++) {
                    z->m_children[z->m_num_children++] = y->m_children[j];
                }
                y->m_num_children -= (m_split_right + 1);
            }

            // insert middle into this and add z as child i+1
            m_impl.insert(m);
            insert_child(i + 1, z);

            // some assertions
            assert(m_children[i] == y);
            assert(m_children[i+1] == z);
            assert(z->m_impl.size() == m_split_right);
            if(!y->is_leaf()) assert(z->m_num_children == m_split_right + 1);
            assert(y->m_impl.size() == m_split_mid);
            if(!y->is_leaf()) assert(y->m_num_children == m_split_mid + 1);
        }
        
        void insert(const key_t key, Observer* obs) {
            assert(!is_full());
            
            if(is_leaf()) {
                // we're at a leaf, insert
                m_impl.insert(key);
                
                // notify observer
                obs->key_inserted(key, *this);
            } else {
                // find the child to descend into
                const auto r = m_impl.predecessor(key);
                size_t i = r.exists ? r.pos + 1 : 0;
                
                if(m_children[i]->is_full()) {
                    // it's full, split it up first
                    split_child(i);

                    // we may have to increase the index of the child to descend into
                    if(key > m_impl[i]) ++i;
                }

                // descend into non-full child
                m_children[i]->insert(key, obs);
            }
        }

        bool remove(const key_t key, Observer* obs) {
            assert(!is_empty());

            if(is_leaf()) {
                // leaf - simply remove
                const bool result = m_impl.remove(key);
                
                // notify observer
                if(result) obs->key_removed(key, *this);
                
                return result;
            } else {
                // find the item, or the child to descend into
                const auto r = m_impl.predecessor(key);
                size_t i = r.exists ? r.pos + 1 : 0;
                
                if(r.exists && m_impl[r.pos] == key) {
                    // key is contained in this internal node
                    assert(i < m_degree);

                    Node* y = m_children[i-1];
                    const size_t ysize = y->size();
                    Node* z = m_children[i];
                    const size_t zsize = z->size();
                    
                    if(ysize >= m_deletion_threshold) {
                        // find predecessor of key in y's subtree - i.e., the maximum
                        Node* c = y;
                        while(!c->is_leaf()) {
                            c = c->m_children[c->m_num_children-1];
                        }
                        const key_t key_pred = c->m_impl[c->size()-1];

                        // replace key by predecssor in this node
                        m_impl.remove(key);
                        m_impl.insert(key_pred);

                        // recursively delete key_pred from y
                        y->remove(key_pred, obs);
                    } else if(zsize >= m_deletion_threshold) {
                        // find successor of key in z's subtree - i.e., its minimum
                        Node* c = z;
                        while(!c->is_leaf()) {
                            c = c->m_children[0];
                        }
                        const key_t key_succ = c->m_impl[0];

                        // replace key by successor in this node
                        m_impl.remove(key);
                        m_impl.insert(key_succ);

                        // recursively delete key_succ from z
                        z->remove(key_succ, obs);
                    } else {
                        // assert balance
                        assert(ysize == m_deletion_threshold - 1);
                        assert(zsize == m_deletion_threshold - 1);

                        // remove key from this node
                        m_impl.remove(key);

                        // merge key and all of z into y
                        {
                            // insert key and keys of z into y
                            y->m_impl.insert(key);
                            for(size_t j = 0; j < zsize; j++) {
                                y->m_impl.insert(z->m_impl[j]);
                            }

                            // move m_children from z to y
                            if(!z->is_leaf()) {
                                assert(!y->is_leaf()); // the sibling of an inner node cannot be a leaf
                                
                                size_t next_child = y->m_num_children;
                                for(size_t j = 0; j < z->m_num_children; j++) {
                                    y->m_children[next_child++] = z->m_children[j];
                                }
                                y->m_num_children = next_child;
                                z->m_num_children = 0;
                            }
                        }

                        // delete z
                        remove_child(i);
                        delete z;

                        // recursively delete key from y
                        y->remove(key, obs);
                    }
                    return true;
                } else {
                    // get i-th child
                    Node* c = m_children[i];

                    if(c->size() < m_deletion_threshold) {
                        // preprocess child so we can safely remove from it
                        assert(c->size() == m_deletion_threshold - 1);
                        
                        // get siblings
                        Node* left  = i > 0 ? m_children[i-1] : nullptr;
                        Node* right = i < m_num_children-1 ? m_children[i+1] : nullptr;
                        
                        if(left && left->size() >= m_deletion_threshold) {
                            // there is a left child, so there must be a splitter
                            assert(i > 0);
                            
                            // retrieve splitter and move it into c
                            const key_t splitter = m_impl[i-1];
                            assert(key > splitter); // sanity
                            m_impl.remove(splitter);
                            c->m_impl.insert(splitter);
                            
                            // move largest key from left sibling to this node
                            const key_t llargest = left->m_impl[left->size()-1];
                            assert(splitter > llargest); // sanity
                            left->m_impl.remove(llargest);
                            m_impl.insert(llargest);
                            
                            // move rightmost child of left sibling to c
                            if(!left->is_leaf()) {
                                Node* lrightmost = left->m_children[left->m_num_children-1];
                                left->remove_child(left->m_num_children-1);
                                c->insert_child(0, lrightmost);
                            }
                        } else if(right && right->size() >= m_deletion_threshold) {
                            // there is a right child, so there must be a splitter
                            assert(i < m_impl.size());
                            
                            // retrieve splitter and move it into c
                            const key_t splitter = m_impl[i];
                            assert(key < splitter); // sanity
                            m_impl.remove(splitter);
                            c->m_impl.insert(splitter);
                            
                            // move smallest key from right sibling to this node
                            const key_t rsmallest = right->m_impl[0];
                            assert(rsmallest > splitter); // sanity
                            right->m_impl.remove(rsmallest);
                            m_impl.insert(rsmallest);
                            
                            // move leftmost child of right sibling to c
                            if(!right->is_leaf()) {
                                Node* rleftmost = right->m_children[0];
                                right->remove_child(0);
                                c->insert_child(c->m_num_children, rleftmost);
                            }
                        } else {
                            // this node is not a leaf and is not empty, so there must be at least one sibling to the child
                            assert(left != nullptr || right != nullptr);
                            assert(left == nullptr || left->size() == m_deletion_threshold - 1);
                            assert(right == nullptr || right->size() == m_deletion_threshold - 1);
                            
                            // select the sibling and corresponding splitter to mergre with
                            if(right != nullptr) {
                                // merge child with right sibling
                                const key_t splitter = m_impl[i];
                                assert(key < splitter); // sanity
                                
                                // move splitter into child as new median
                                m_impl.remove(splitter);
                                c->m_impl.insert(splitter);
                                
                                // move keys right sibling to child
                                for(size_t j = 0; j < right->size(); j++) {
                                    c->m_impl.insert(right->m_impl[j]);
                                }
                                
                                if(!right->is_leaf()) {
                                    assert(!c->is_leaf()); // the sibling of an inner node cannot be a leaf
                                    
                                    // append m_children of right sibling to child
                                    size_t next_child = c->m_num_children;
                                    for(size_t j = 0; j < right->m_num_children; j++) {
                                        c->m_children[next_child++] = right->m_children[j];
                                    }
                                    c->m_num_children = next_child;
                                    right->m_num_children = 0;
                                }
                                
                                // delete right sibling
                                remove_child(i+1);
                                delete right;
                            } else {
                                // merge child with left sibling
                                const key_t splitter = m_impl[i-1];
                                assert(key > splitter); // sanity
                                
                                // move splitter into child as new median
                                m_impl.remove(splitter);
                                c->m_impl.insert(splitter);
                                
                                // move keys left sibling to child
                                for(size_t j = 0; j < left->size(); j++) {
                                    c->m_impl.insert(left->m_impl[j]);
                                }
                                
                                if(!left->is_leaf()) {
                                    assert(!c->is_leaf()); // the sibling of an inner node cannot be a leaf
                                    
                                    // move m_children of child to the back
                                    size_t next_child = left->m_num_children;
                                    for(size_t j = 0; j < c->m_num_children; j++) {
                                        c->m_children[next_child++] = c->m_children[j];
                                    }
                                    c->m_num_children = next_child;
                                    
                                    // prepend m_children of left sibling to child
                                    for(size_t j = 0; j < left->m_num_children; j++) {
                                        c->m_children[j] = left->m_children[j];
                                    }
                                    left->m_num_children = 0;
                                }
                                
                                // delete left sibling
                                remove_child(i-1);
                                delete left;
                            }
                        }
                    }
                    
                    // remove from subtree
                    return c->remove(key, obs);
                }
            }
        }

#ifndef NDEBUG
        size_t print(size_t num, const size_t level) const {
            for(size_t i = 0; i < level; i++) std::cout << "    ";
            std::cout << "node " << num << ": ";
            for(size_t i = 0; i < m_impl.size(); i++) std::cout << m_impl[i] << " ";
            std::cout << std::endl;
            ++num;
            for(size_t i = 0; i < m_num_children; i++) num = m_children[i]->print(num, level+1);
            return num;
        }

        void verify() const {
            if(!is_leaf()) {
                // make sure there is one more child than splitters in the node
                assert(m_num_children == size() + 1);
                
                for(size_t i = 0; i < size(); i++) {
                    // the largest value of the i-th subtree must be smaller than the i-th splitter
                    Node* c = m_children[i];
                    while(!c->is_leaf()) {
                        c = c->m_children[c->m_num_children-1];
                    }
                    assert(c->m_impl[c->size()-1] < m_impl[i]);
                }
                
                // the smallest value of the last subtree must be larger than the last splitter
                {
                    Node* c = m_children[size()];
                    while(!c->is_leaf()) {
                        c = c->m_children[0];
                    }
                    assert(c->m_impl[0] > m_impl[size() - 1]);
                }
                
                // verify m_children
                for(size_t i = 0; i < m_num_children; i++) {
                    m_children[i]->verify();
                }
            }
        }
#endif
    } __attribute__((__packed__));
    
private:
    size_t m_size;
    Node* m_root;
    
    Observer m_default_observer;
    Observer* m_observer;

public:
    BTree() : m_size(0), m_root(new Node()), m_observer(&m_default_observer) {
    }

    ~BTree() {
        delete m_root;
    }

    /// \brief Finds the \em value of the predecessor of the specified key in the tree.
    /// \param x the key in question
    KeyResult<key_t> predecessor(const key_t x) const {
        Node* node = m_root;
        PosResult r;
        
        bool exists = false;
        key_t value;
        
        r = node->m_impl.predecessor(x);
        while(!node->is_leaf()) {
            exists = exists || r.exists;
            if(r.exists) {
                value = node->m_impl[r.pos];
                if(value == x) {
                    return { true, x };
                }
            }
               
            const size_t i = r.exists ? r.pos + 1 : 0;
            node = node->m_children[i];
            r = node->m_impl.predecessor(x);
        }
        
        exists = exists || r.exists;
        if(r.exists) value = node->m_impl[r.pos];
        
        return { exists, value };
    }
    
    /// \brief Finds the \em value of the successor of the specified key in the tree.
    /// \param x the key in question
    KeyResult<key_t> successor(const key_t x) const {
        Node* node = m_root;
        PosResult r;
        
        bool exists = false;
        key_t value;
        
        r = node->m_impl.successor(x);
        while(!node->is_leaf()) {
            exists = exists || r.exists;
            if(r.exists) {
                value = node->m_impl[r.pos];
                if(value == x) {
                    return { true, x };
                }
            }

            const size_t i = r.exists ? r.pos : node->m_num_children - 1;
            node = node->m_children[i];
            r = node->m_impl.successor(x);
        }
        
        exists = exists || r.exists;
        if(r.exists) value = node->m_impl[r.pos];
        
        return { exists, value };
    }

    /// \brief Tests whether the given key is contained in the trie.
    /// \param x the key in question
    inline bool contains(const key_t x) const {
        if(tdc_unlikely(size() == 0)) return false;
        auto r = predecessor(x);
        return r.exists && r.key == x;
    }

    /// \brief Reports the minimum key in the trie.
    key_t min() const {
        Node* node = m_root;
        while(!node->is_leaf()) {
            node = node->m_children[0];
        }
        return node->m_impl[0];
    }

    /// \brief Reports the maximum key in the trie.
    key_t max() const {
        Node* node = m_root;
        while(!node->is_leaf()) {
            node = node->m_children[node->m_num_children - 1];
        }
        return node->m_impl[node->size() - 1];
    }

    /// \brief Inserts the specified key.
    /// \param key the key to insert
    void insert(const key_t key) {
        if(m_root->is_full()) {
            // root is full, split it up
            Node* new_root = new Node();
            new_root->insert_child(0, m_root);

            m_root = new_root;
            m_root->split_child(0);
        }
        m_root->insert(key, m_observer);
        ++m_size;
    }

    /// \brief Removes the specified key.
    /// \param key the key to remove
    /// \return whether the item was found and removed
    bool remove(const key_t key) {
        assert(m_size > 0);
        
        bool result = m_root->remove(key, m_observer);
        
        if(result) {
            --m_size;
        }
        
        if(m_root->size() == 0 && m_root->m_num_children > 0) {
            assert(m_root->m_num_children == 1);
            
            // root is now empty but it still has a child, make that new root
            Node* new_root = m_root->m_children[0];
            m_root->m_num_children = 0;
            delete m_root;
            m_root = new_root;
        }
        return result;
    }
    
    /// \brief Returns the current size of the underlying octrie.
    inline size_t size() const {
        return m_size;
    }
    
    /// \brief Returns a pointer to the root node.
    inline const Node* root() const {
        return m_root;
    }
    
    /// \brief Sets the observer.
    inline void set_observer(Observer* obs) {
        m_observer = obs ? obs : &m_default_observer;
    }
    
#ifndef NDEBUG
    void print() const {
        m_root->print(0, 0);
    }
    
    void verify() const {
        m_root->verify();
    }
#endif
};


}}} // namespace tdc::pred::dynamic
