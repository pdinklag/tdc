#pragma once

#include <tdc/code/huff/huffman_coder_base.hpp>

namespace tdc {
namespace code {

class AdaptiveHuffmanCoderBase : public HuffmanCoderBase {
protected:
    Node* rank_map_[MAX_NODES];

    inline void set_rank(Node* v, const index_t rank, const bool initial) {
        assert(rank < MAX_NODES);

        if(!initial) rank_map_[v->rank] = nullptr;
        v->rank = rank;
        rank_map_[rank] = v;
    }

    // assumes that queue contains all the leaves!
    // same as Huffman52Coder::build_tree, except we also set the ranks here
    inline void build_tree(PriorityQueue& queue) {
        assert(!queue.empty());

        size_t next_rank = 0;
        const size_t sigma = queue.size();
        for(size_t i = 0; i < sigma - 1; i++) {
            // get the next two nodes from the priority queue
            Node* l = queue.top(); queue.pop();
            Node* r = queue.top(); queue.pop();

            // make sure l has the smaller weight
            if(r->weight < l->weight) {
                std::swap(l, r);
            }

            // determine ranks
            set_rank(l, next_rank++, true);
            set_rank(r, next_rank++, true);

            // create a new node as parent of l and r
            Node* v = node(num_nodes_++);
            *v = Node { l->weight + r->weight, 0, nullptr, 0, l, r, 0 };

            l->parent = v;
            l->bit = 0;
            r->parent = v;
            r->bit = 1;

            queue.push(v);
        }

        root_ = queue.top(); queue.pop();
        set_rank(root_, next_rank++, true);
        assert(queue.empty());
    }

    // same as HuffmanBase, but using set_rank
    inline void replace(Node* u, Node* v) {
        u->parent = v->parent;
        set_rank(u, v->rank, false);
        u->bit = v->bit;

        if(u->parent) {
            if(u->bit) {
                u->parent->right = u;
            } else {
                u->parent->left = u;
            }
        }
    }

    using HuffmanCoderBase::HuffmanCoderBase;
};

}} // namespace tdc::code
