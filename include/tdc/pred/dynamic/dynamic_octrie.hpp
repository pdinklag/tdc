#pragma once

#include "dynamic_fusion_node_8.hpp"
#include <functional>

namespace tdc {
namespace pred {
namespace dynamic {

/// \brief A node dynamic octrie.
class DynamicOctrie {
private:
    struct Node {
        DynamicFusionNode8 fnode;
        uint8_t num_children;
        Node** children;
    
        inline bool is_leaf() const {
            return children == nullptr;
        }
        
        inline size_t size() const {
            return fnode.size();
        }
        
        inline bool is_empty() const {
            return size() == 0;
        }

        inline bool is_full() const {
            return size() == 8;
        }

        Node();
        ~Node();

        Node(const Node&) = default;
        Node(Node&&) = default;
        Node& operator=(const Node&) = default;
        Node& operator=(Node&&) = default;

        void insert_child(const size_t i, Node* node);
        void remove_child(const size_t i);

        void split_child(const size_t i);
        void insert(const uint64_t key);
        bool remove(const uint64_t key);

#ifndef NDEBUG
        size_t print(size_t num, const size_t level) const;
        void verify() const;
#endif
    } __attribute__((__packed__));

    size_t m_size;
    Node* m_root;

public:
    /// \brief Constructs an empty octrie node.
    DynamicOctrie();
    ~DynamicOctrie();

    DynamicOctrie(const DynamicOctrie&) = delete;
    DynamicOctrie(DynamicOctrie&&) = delete;
    DynamicOctrie& operator=(const DynamicOctrie&) = delete;
    DynamicOctrie& operator=(DynamicOctrie&&) = delete;

    /// \brief Finds the \em value of the predecessor of the specified key in the trie.
    /// \param x the key in question
    Result predecessor(const uint64_t x) const;

    /// \brief Tests whether the given key is contained in the trie.
    /// \param x the key in question
    inline bool contains(const uint64_t x) const {
        auto r = predecessor(x);
        return r.exists && r.pos == x;
    }

    /// \brief Reports the minimum key in the trie.
    uint64_t min() const;

    /// \brief Reports the maximum key in the trie.
    uint64_t max() const;

    /// \brief Inserts the specified key.
    /// \param key the key to insert
    void insert(const uint64_t key);

    /// \brief Removes the specified key.
    /// \param key the key to remove
    /// \return whether the item was found and removed
    bool remove(const uint64_t key);
    
    /// \brief Returns the current size of the underlying octrie.
    inline size_t size() const {
        return m_size;
    }
    
#ifndef NDEBUG
    void print() const;
    void verify() const;
#endif
};

}}} // namespace tdc::pred::dynamic
