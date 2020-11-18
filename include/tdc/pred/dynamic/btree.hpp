#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>

#include <tdc/pred/result.hpp>

namespace tdc {
namespace pred {
namespace dynamic {

/// \brief A B-Tree.
/// \tparam key_t the key type
/// \tparam degree the maximum number of children per node
/// \tparam node_impl_t node implementation; must be sorted and support array access, size, insert, remove and predecessor
template<typename key_t, size_t m_degree, typename node_impl_t>
class BTree {
private:
    static_assert((m_degree % 2) == 1); // we only allow odd maximum degrees for the sake of implementation simplicity
    static_assert(m_degree > 1);
    static_assert(m_degree < 256);
    
    static constexpr size_t m_max_node_keys = m_degree - 1;
    static constexpr size_t m_split_right = m_max_node_keys / 2;
    static constexpr size_t m_split_mid = m_split_right - 1;
    static constexpr size_t m_deletion_threshold = m_degree / 2;

    struct Node {
        node_impl_t impl;
        uint8_t num_children;
        Node** children;
    
        inline bool is_leaf() const {
            return children == nullptr;
        }
        
        inline size_t size() const {
            return impl.size();
        }
        
        inline bool is_empty() const {
            return size() == 0;
        }

        inline bool is_full() const {
            return size() == m_max_node_keys;
        }

        Node() : children(nullptr), num_children(0) {
        }
        
        ~Node() {
            for(size_t i = 0; i < num_children; i++) {
                delete children[i];
            }
            if(children) delete[] children;
        }

        Node(const Node&) = default;
        Node(Node&&) = default;
        Node& operator=(const Node&) = default;
        Node& operator=(Node&&) = default;

        inline void alloc_children() {
            if(children == nullptr) {
                children = new Node*[m_degree];
            }
        }

        void insert_child(const size_t i, Node* node) {
            assert(i <= num_children);
            assert(num_children < m_degree);
            alloc_children();
            
            // insert
            for(size_t j = num_children; j > i; j--) {
                children[j] = children[j-1];
            }
            children[i] = node;
            ++num_children;
        }
        
        void remove_child(const size_t i) {
            assert(num_children > 0);
            assert(i <= num_children);

            for(size_t j = i; j < num_children-1; j++) {
                children[j] = children[j+1];
            }
            children[num_children-1] = nullptr;
            --num_children;
            
            if(num_children == 0) {
                delete[] children;
                children = nullptr;
            }
        }

        void split_child(const size_t i) {
            assert(!is_full());
            
            Node* y = children[i];
            assert(y->is_full());

            // allocate new node
            Node* z = new Node();

            // get the middle value
            const key_t m = y->impl[m_split_mid];

            // move the keys larger than middle from y to z and remove the middle
            {
                key_t buf[m_split_right];
                for(size_t j = 0; j < m_split_right; j++) {
                    buf[j] = y->impl[j + m_split_right];
                    z->impl.insert(buf[j]);
                }
                for(size_t j = 0; j < m_split_right; j++) {
                    y->impl.remove(buf[j]);
                }
                y->impl.remove(m);
            }

            // move the children right of middle from y to z
            if(!y->is_leaf()) {
                z->alloc_children();
                for(size_t j = m_split_right; j <= m_max_node_keys; j++) {
                    z->children[z->num_children++] = y->children[j];
                }
                y->num_children -= (m_split_right + 1);
            }

            // insert middle into this and add z as child i+1
            impl.insert(m);
            insert_child(i + 1, z);

            // some assertions
            assert(children[i] == y);
            assert(children[i+1] == z);
            assert(z->impl.size() == m_split_right);
            if(!y->is_leaf()) assert(z->num_children == m_split_right + 1);
            assert(y->impl.size() == m_split_mid);
            if(!y->is_leaf()) assert(y->num_children == m_split_mid + 1);
        }
        
        void insert(const key_t key) {
            assert(!is_full());
            
            if(is_leaf()) {
                // we're at a leaf, insert
                impl.insert(key);
            } else {
                // find the child to descend into
                const auto r = impl.predecessor(key);
                size_t i = r.exists ? r.pos + 1 : 0;
                
                if(children[i]->is_full()) {
                    // it's full, split it up first
                    split_child(i);

                    // we may have to increase the index of the child to descend into
                    if(key > impl[i]) ++i;
                }

                // descend into non-full child
                children[i]->insert(key);
            }
        }

        bool remove(const key_t key) {
            assert(!is_empty());

            if(is_leaf()) {
                // leaf - simply remove
                return impl.remove(key);
            } else {
                // find the item, or the child to descend into
                const auto r = impl.predecessor(key);
                size_t i = r.exists ? r.pos + 1 : 0;
                
                if(r.exists && impl[r.pos] == key) {
                    // key is contained in this internal node
                    assert(i < m_degree);

                    Node* y = children[i-1];
                    const size_t ysize = y->size();
                    Node* z = children[i];
                    const size_t zsize = z->size();
                    
                    if(ysize >= m_deletion_threshold) {
                        // find predecessor of key in y's subtree - i.e., the maximum
                        Node* c = y;
                        while(!c->is_leaf()) {
                            c = c->children[c->num_children-1];
                        }
                        const key_t key_pred = c->impl[c->size()-1];

                        // replace key by predecssor in this node
                        impl.remove(key);
                        impl.insert(key_pred);

                        // recursively delete key_pred from y
                        y->remove(key_pred);
                    } else if(zsize >= m_deletion_threshold) {
                        // find successor of key in z's subtree - i.e., its minimum
                        Node* c = z;
                        while(!c->is_leaf()) {
                            c = c->children[0];
                        }
                        const key_t key_succ = c->impl[0];

                        // replace key by successor in this node
                        impl.remove(key);
                        impl.insert(key_succ);

                        // recursively delete key_succ from z
                        z->remove(key_succ);
                    } else {
                        // assert balance
                        assert(ysize == m_deletion_threshold - 1);
                        assert(zsize == m_deletion_threshold - 1);

                        // remove key from this node
                        impl.remove(key);

                        // merge key and all of z into y
                        {
                            // insert key and keys of z into y
                            y->impl.insert(key);
                            for(size_t j = 0; j < zsize; j++) {
                                y->impl.insert(z->impl[j]);
                            }

                            // move children from z to y
                            if(!z->is_leaf()) {
                                assert(!y->is_leaf()); // the sibling of an inner node cannot be a leaf
                                
                                size_t next_child = y->num_children;
                                for(size_t j = 0; j < z->num_children; j++) {
                                    y->children[next_child++] = z->children[j];
                                }
                                y->num_children = next_child;
                                z->num_children = 0;
                            }
                        }

                        // delete z
                        remove_child(i);
                        delete z;

                        // recursively delete key from y
                        y->remove(key);
                    }
                    return true;
                } else {
                    // get i-th child
                    Node* c = children[i];

                    if(c->size() < m_deletion_threshold) {
                        // preprocess child so we can safely remove from it
                        assert(c->size() == m_deletion_threshold - 1);
                        
                        // get siblings
                        Node* left  = i > 0 ? children[i-1] : nullptr;
                        Node* right = i < num_children-1 ? children[i+1] : nullptr;
                        
                        if(left && left->size() >= m_deletion_threshold) {
                            // there is a left child, so there must be a splitter
                            assert(i > 0);
                            
                            // retrieve splitter and move it into c
                            const key_t splitter = impl[i-1];
                            assert(key > splitter); // sanity
                            impl.remove(splitter);
                            c->impl.insert(splitter);
                            
                            // move largest key from left sibling to this node
                            const key_t llargest = left->impl[left->size()-1];
                            assert(splitter > llargest); // sanity
                            left->impl.remove(llargest);
                            impl.insert(llargest);
                            
                            // move rightmost child of left sibling to c
                            if(!left->is_leaf()) {
                                Node* lrightmost = left->children[left->num_children-1];
                                left->remove_child(left->num_children-1);
                                c->insert_child(0, lrightmost);
                            }
                        } else if(right && right->size() >= m_deletion_threshold) {
                            // there is a right child, so there must be a splitter
                            assert(i < impl.size());
                            
                            // retrieve splitter and move it into c
                            const key_t splitter = impl[i];
                            assert(key < splitter); // sanity
                            impl.remove(splitter);
                            c->impl.insert(splitter);
                            
                            // move smallest key from right sibling to this node
                            const key_t rsmallest = right->impl[0];
                            assert(rsmallest > splitter); // sanity
                            right->impl.remove(rsmallest);
                            impl.insert(rsmallest);
                            
                            // move leftmost child of right sibling to c
                            if(!right->is_leaf()) {
                                Node* rleftmost = right->children[0];
                                right->remove_child(0);
                                c->insert_child(c->num_children, rleftmost);
                            }
                        } else {
                            // this node is not a leaf and is not empty, so there must be at least one sibling to the child
                            assert(left != nullptr || right != nullptr);
                            assert(left == nullptr || left->size() == m_deletion_threshold - 1);
                            assert(right == nullptr || right->size() == m_deletion_threshold - 1);
                            
                            // select the sibling and corresponding splitter to mergre with
                            if(right != nullptr) {
                                // merge child with right sibling
                                const key_t splitter = impl[i];
                                assert(key < splitter); // sanity
                                
                                // move splitter into child as new median
                                impl.remove(splitter);
                                c->impl.insert(splitter);
                                
                                // move keys right sibling to child
                                for(size_t j = 0; j < right->size(); j++) {
                                    c->impl.insert(right->impl[j]);
                                }
                                
                                if(!right->is_leaf()) {
                                    assert(!c->is_leaf()); // the sibling of an inner node cannot be a leaf
                                    
                                    // append children of right sibling to child
                                    size_t next_child = c->num_children;
                                    for(size_t j = 0; j < right->num_children; j++) {
                                        c->children[next_child++] = right->children[j];
                                    }
                                    c->num_children = next_child;
                                    right->num_children = 0;
                                }
                                
                                // delete right sibling
                                remove_child(i+1);
                                delete right;
                            } else {
                                // merge child with left sibling
                                const key_t splitter = impl[i-1];
                                assert(key > splitter); // sanity
                                
                                // move splitter into child as new median
                                impl.remove(splitter);
                                c->impl.insert(splitter);
                                
                                // move keys left sibling to child
                                for(size_t j = 0; j < left->size(); j++) {
                                    c->impl.insert(left->impl[j]);
                                }
                                
                                if(!left->is_leaf()) {
                                    assert(!c->is_leaf()); // the sibling of an inner node cannot be a leaf
                                    
                                    // move children of child to the back
                                    size_t next_child = left->num_children;
                                    for(size_t j = 0; j < c->num_children; j++) {
                                        c->children[next_child++] = c->children[j];
                                    }
                                    c->num_children = next_child;
                                    
                                    // prepend children of left sibling to child
                                    for(size_t j = 0; j < left->num_children; j++) {
                                        c->children[j] = left->children[j];
                                    }
                                    left->num_children = 0;
                                }
                                
                                // delete left sibling
                                remove_child(i-1);
                                delete left;
                            }
                        }
                    }
                    
                    // remove from subtree
                    return c->remove(key);
                }
            }
        }

#ifndef NDEBUG
        size_t print(size_t num, const size_t level) const {
            for(size_t i = 0; i < level; i++) std::cout << "    ";
            std::cout << "node " << num << ": ";
            for(size_t i = 0; i < impl.size(); i++) std::cout << impl[i] << " ";
            std::cout << std::endl;
            ++num;
            for(size_t i = 0; i < num_children; i++) num = children[i]->print(num, level+1);
            return num;
        }

        void verify() const {
            if(!is_leaf()) {
                // make sure there is one more child than splitters in the node
                assert(num_children == size() + 1);
                
                for(size_t i = 0; i < size(); i++) {
                    // the largest value of the i-th subtree must be smaller than the i-th splitter
                    Node* c = children[i];
                    while(!c->is_leaf()) {
                        c = c->children[c->num_children-1];
                    }
                    assert(c->impl[c->size()-1] < impl[i]);
                }
                
                // the smallest value of the last subtree must be larger than the last splitter
                {
                    Node* c = children[size()];
                    while(!c->is_leaf()) {
                        c = c->children[0];
                    }
                    assert(c->impl[0] > impl[size() - 1]);
                }
                
                // verify children
                for(size_t i = 0; i < num_children; i++) {
                    children[i]->verify();
                }
            }
        }
#endif
    } __attribute__((__packed__));
    
    size_t m_size;
    Node* m_root;

public:
    BTree() : m_size(0), m_root(new Node()) {
    }

    ~BTree() {
        delete m_root;
    }

    /// \brief Finds the \em value of the predecessor of the specified key in the trie.
    /// \param x the key in question
    KeyResult<key_t> predecessor(const key_t x) const {
        Node* node = m_root;
        PosResult r;
        
        bool exists = false;
        key_t value = std::numeric_limits<key_t>::max();
        
        r = node->impl.predecessor(x);
        while(!node->is_leaf()) {
            exists = exists || r.exists;
            if(r.exists) {
                value = node->impl[r.pos];
                if(value == x) {
                    return { true, x };
                }
            }
               
            const size_t i = r.exists ? r.pos + 1 : 0;
            node = node->children[i];
            r = node->impl.predecessor(x);
        }
        
        exists = exists || r.exists;
        if(r.exists) value = node->impl[r.pos];
        
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
            node = node->children[0];
        }
        return node->impl[0];
    }

    /// \brief Reports the maximum key in the trie.
    key_t max() const {
        Node* node = m_root;
        while(!node->is_leaf()) {
            node = node->children[node->num_children - 1];
        }
        return node->impl[node->size() - 1];
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
        m_root->insert(key);
        ++m_size;
    }

    /// \brief Removes the specified key.
    /// \param key the key to remove
    /// \return whether the item was found and removed
    bool remove(const key_t key) {
        assert(m_size > 0);
        
        bool result = m_root->remove(key);
        
        if(result) {
            --m_size;
        }
        
        if(m_root->size() == 0 && m_root->num_children > 0) {
            assert(m_root->num_children == 1);
            
            // root is now empty but it still has a child, make that new root
            Node* new_root = m_root->children[0];
            m_root->num_children = 0;
            delete m_root;
            m_root = new_root;
        }
        return result;
    }
    
    /// \brief Returns the current size of the underlying octrie.
    inline size_t size() const {
        return m_size;
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
