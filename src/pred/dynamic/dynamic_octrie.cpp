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
    delete[] children;
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
        // find the child to descend to
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
    // TODO
    return false;
}
