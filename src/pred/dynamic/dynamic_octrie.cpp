#include <cassert>
#include <iostream> // FIXME DEBUG

#include <tdc/pred/dynamic/dynamic_octrie.hpp>

using namespace tdc::pred::dynamic;

DynamicOctrie::DynamicOctrie() : m_size(0), m_root(new Node()) {
}

DynamicOctrie::~DynamicOctrie() {
    delete m_root;
}

DynamicOctrie::Node::Node() : children(nullptr), num_children(0) {
}

DynamicOctrie::Node::~Node() {
    for(size_t i = 0; i < num_children; i++) {
        delete children[i];
    }
    if(children) delete[] children;
}

#ifndef NDEBUG
size_t DynamicOctrie::Node::print(size_t num, const size_t level) const {
    for(size_t i = 0; i < level; i++) std::cout << "    ";
    std::cout << "node " << num << ": ";
    for(size_t i = 0; i < fnode.size(); i++) std::cout << fnode[i] << " ";
    std::cout << std::endl;
    ++num;
    for(size_t i = 0; i < num_children; i++) num = children[i]->print(num, level+1);
    return num;
}

void DynamicOctrie::Node::verify() const {
    if(!is_leaf()) {
        // make sure there is one more child than splitters in the node
        assert(num_children == size() + 1);
        
        for(size_t i = 0; i < size(); i++) {
            // the largest value of the i-th subtree must be smaller than the i-th splitter
            Node* c = children[i];
            while(!c->is_leaf()) {
                c = c->children[c->num_children-1];
            }
            assert(c->fnode[c->size()-1] < fnode[i]);
        }
        
        // the smallest value of the last subtree must be larger than the last splitter
        {
            Node* c = children[size()];
            while(!c->is_leaf()) {
                c = c->children[0];
            }
            assert(c->fnode[0] > fnode[size() - 1]);
        }
        
        // verify children
        for(size_t i = 0; i < num_children; i++) {
            children[i]->verify();
        }
    }
}
#endif

void DynamicOctrie::Node::insert_child(const size_t i, Node* node) {
    assert(i <= num_children);
    
    if(children == nullptr) {
        children = new Node*[9];
    }
    
    // insert
    for(size_t j = num_children; j > i; j--) {
        children[j] = children[j-1];
    }
    children[i] = node;
    ++num_children;
}

void DynamicOctrie::Node::remove_child(const size_t i) {
    assert(num_children > 0);
    assert(i <= num_children);

    for(size_t j = i; j < num_children-1; j++) {
        children[j] = children[j+1];
    }
    children[num_children-1] = nullptr;
    --num_children;
}

void DynamicOctrie::Node::split_child(const size_t i) {
    assert(!is_full());
    
    Node* y = children[i];
    assert(y->is_full());

    // allocate new node
    Node* z = new Node();

    // get the middle value
    const uint64_t m = y->fnode.select(3);

    // move the 4 largest keys from y to z and remove the middle
    {
        uint64_t buf[4];
        for(size_t j = 0; j < 4; j++) {
            buf[j] = y->fnode[j + 4];
            z->fnode.insert(buf[j]);
        }
        for(size_t j = 0; j < 4; j++) {
            y->fnode.remove(buf[j]);
        }
        y->fnode.remove(m);
    }

    // move the last 5 children from y to z
    if(!y->is_leaf()) {
        for(size_t j = 4; j <= 8; j++) {
            z->insert_child(z->num_children, y->children[j]);
        }
        y->num_children -= 5;
    }

    // insert middle into this and add z as child i+1
    fnode.insert(m);
    insert_child(i + 1, z);

    // some assertions
    assert(children[i] == y);
    assert(children[i+1] == z);
    assert(z->fnode.size() == 4);
    if(!y->is_leaf()) assert(z->num_children == 5);
    assert(y->fnode.size() == 3);
    if(!y->is_leaf()) assert(y->num_children == 4);
}

void DynamicOctrie::Node::insert(const uint64_t key) {
    assert(!is_full());
    
    if(is_leaf()) {
        // we're at a leaf, insert
        fnode.insert(key);
    } else {
        // find the child to descend into
        const auto r = fnode.predecessor(key);
        size_t i = r.exists ? r.pos + 1 : 0;
        
        if(children[i]->is_full()) {
            // it's full, split it up first
            split_child(i);

            // we may have to increase the index of the child to descend into
            if(key > fnode.select(i)) ++i;
        }

        // descend into non-full child
        children[i]->insert(key);
    }
}

bool DynamicOctrie::Node::remove(const uint64_t key) {
    static constexpr size_t DELETION_THRESHOLD = 4;
    
    assert(!is_empty());

    if(is_leaf()) {
        // leaf - simply remove
        return fnode.remove(key);
    } else {
        // find the item, or the child to descend into
        const auto r = fnode.predecessor(key);
        size_t i = r.exists ? r.pos + 1 : 0;
        
        if(r.exists && fnode[r.pos] == key) {
            // key is contained in this internal node
            assert(i < 9);

            Node* y = children[i-1];
            const size_t ysize = y->size();
            Node* z = children[i];
            const size_t zsize = z->size();
            
            if(ysize >= DELETION_THRESHOLD) {
                // find predecessor of key in y's subtree - i.e., the maximum
                Node* c = y;
                while(!c->is_leaf()) {
                    c = c->children[c->num_children-1];
                }
                const uint64_t key_pred = c->fnode[c->size()-1];

                // replace key by predecssor in this node
                fnode.remove(key);
                fnode.insert(key_pred);

                // recursively delete key_pred from y
                y->remove(key_pred);
            } else if(zsize >= DELETION_THRESHOLD) {
                // find successor of key in z's subtree - i.e., its minimum
                Node* c = z;
                while(!c->is_leaf()) {
                    c = c->children[0];
                }
                const uint64_t key_succ = c->fnode[0];

                // replace key by successor in this node
                fnode.remove(key);
                fnode.insert(key_succ);

                // recursively delete key_succ from z
                z->remove(key_succ);
            } else {
                // assert balance
                assert(ysize == DELETION_THRESHOLD - 1);
                assert(zsize == DELETION_THRESHOLD - 1);

                // remove key from this node
                fnode.remove(key);

                // merge key and all of z into y
                {
                    // insert key and keys of z into y
                    y->fnode.insert(key);
                    for(size_t j = 0; j < zsize; j++) {
                        y->fnode.insert(z->fnode[j]);
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

            if(c->size() < DELETION_THRESHOLD) {
                // preprocess child so we can safely remove from it
                assert(c->size() == DELETION_THRESHOLD - 1);
                
                // get siblings
                Node* left  = i > 0 ? children[i-1] : nullptr;
                Node* right = i < num_children-1 ? children[i+1] : nullptr;
                
                if(left && left->size() >= DELETION_THRESHOLD) {
                    // there is a left child, so there must be a splitter
                    assert(i > 0);
                    
                    // retrieve splitter and move it into c
                    const uint64_t splitter = fnode[i-1];
                    assert(key > splitter); // sanity
                    fnode.remove(splitter);
                    c->fnode.insert(splitter);
                    
                    // move largest key from left sibling to this node
                    const uint64_t llargest = left->fnode[left->size()-1];
                    assert(splitter > llargest); // sanity
                    left->fnode.remove(llargest);
                    fnode.insert(llargest);
                    
                    // move rightmost child of left sibling to c
                    if(!left->is_leaf()) {
                        Node* lrightmost = left->children[left->num_children-1];
                        left->remove_child(left->num_children-1);
                        c->insert_child(0, lrightmost);
                    }
                } else if(right && right->size() >= DELETION_THRESHOLD) {
                    // there is a right child, so there must be a splitter
                    assert(i < fnode.size());
                    
                    // retrieve splitter and move it into c
                    const uint64_t splitter = fnode[i];
                    assert(key < splitter); // sanity
                    fnode.remove(splitter);
                    c->fnode.insert(splitter);
                    
                    // move smallest key from right sibling to this node
                    const uint64_t rsmallest = right->fnode[0];
                    assert(rsmallest > splitter); // sanity
                    right->fnode.remove(rsmallest);
                    fnode.insert(rsmallest);
                    
                    // move leftmost child of right sibling to c
                    if(!right->is_leaf()) {
                        Node* rleftmost = right->children[0];
                        right->remove_child(0);
                        c->insert_child(c->num_children, rleftmost);
                    }
                } else {
                    // this node is not a leaf and is not empty, so there must be at least one sibling to the child
                    assert(left != nullptr || right != nullptr);
                    assert(left == nullptr || left->size() == DELETION_THRESHOLD - 1);
                    assert(right == nullptr || right->size() == DELETION_THRESHOLD - 1);
                    
                    // select the sibling and corresponding splitter to mergre with
                    if(right != nullptr) {
                        // merge child with right sibling
                        const uint64_t splitter = fnode[i];
                        assert(key < splitter); // sanity
                        
                        // move splitter into child as new median
                        fnode.remove(splitter);
                        c->fnode.insert(splitter);
                        
                        // move keys right sibling to child
                        for(size_t j = 0; j < right->size(); j++) {
                            c->fnode.insert(right->fnode[j]);
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
                        const uint64_t splitter = fnode[i-1];
                        assert(key > splitter); // sanity
                        
                        // move splitter into child as new median
                        fnode.remove(splitter);
                        c->fnode.insert(splitter);
                        
                        // move keys left sibling to child
                        for(size_t j = 0; j < left->size(); j++) {
                            c->fnode.insert(left->fnode[j]);
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

Result DynamicOctrie::predecessor(const uint64_t x) const {
    Node* node = m_root;
    Result r;
    
    bool exists = false;
    uint64_t value = UINT64_MAX;
    
    r = node->fnode.predecessor(x);
    while(!node->is_leaf()) {
        exists = exists || r.exists;
        if(r.exists) {
            value = node->fnode[r.pos];
            if(value == x) {
                return { true, x };
            }
        }
           
        const size_t i = r.exists ? r.pos + 1 : 0;
        node = node->children[i];
        r = node->fnode.predecessor(x);
    }
    
    exists = exists || r.exists;
    if(r.exists) value = node->fnode[r.pos];
    
    return { exists, value };
}

#ifndef NDEBUG
void DynamicOctrie::print() const {
    m_root->print(0, 0);
}

void DynamicOctrie::verify() const {
    m_root->verify();
}
#endif

void DynamicOctrie::insert(const uint64_t key) {
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

bool DynamicOctrie::remove(const uint64_t key) {
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
