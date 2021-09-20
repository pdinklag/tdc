#pragma once

#include <cassert>
#include <queue>
#include <utility>
#include <vector>

#include <tdc/code/coder.hpp>
#include <tdc/util/index.hpp>

namespace tdc {
namespace code {

class HuffmanCoderBase : public Coder {
protected:
    using Char = uint8_t;
    static constexpr size_t MAX_SYMS = UINT8_MAX;
    static constexpr size_t MAX_NODES = 2 * (MAX_SYMS + 1); // 2 * (all bytes + NYT)

    struct Node {
        index_t weight;
        index_t rank; // only used by adaptive algorithms

        Node* parent;
        bool bit;

        Node* left;
        Node* right;

        Char sym;

        inline bool is_leaf() {
            return !(left || right);
        }
    };

    struct NodePriorityComparator {
        inline bool operator()(Node* a, Node* b) {
            return a->weight >= b->weight;
        };
    };

    using PriorityQueue = std::priority_queue<Node*, std::vector<Node*>, NodePriorityComparator>;

    Node    nodes_[MAX_NODES];
    index_t num_nodes_;

    Node* leaves_[MAX_SYMS];
    Node* root_;

    inline HuffmanCoderBase() {
    }

    inline Node* node(const size_t v) {
        assert(v < MAX_NODES);
        return &nodes_[v];
    }

    inline const Node* node(const size_t v) const {
        assert(v < MAX_NODES);
        return &nodes_[v];
    }

    inline PriorityQueue init_leaves(const std::string& s) {
        // init leaves
        for(Char c : s) {
            if(leaves_[c]) {
                // already seen, increase weight
                ++leaves_[c]->weight;
            } else {
                // first time this symbol occurs, create leaf
                Node* q = node(num_nodes_++);
                *q = Node { 1, 0, nullptr, 0, nullptr, nullptr, c };
                leaves_[c] = q;
            }
        }

        // insert leaves into priority queue
        PriorityQueue queue(NodePriorityComparator{});
        for(size_t c = 0; c < MAX_SYMS; c++) {
            Node* q = leaves_[c];
            if(q) {
                assert(q->weight > 0);
                queue.push(q);
            }
        }

        return queue;
    }

    template<typename SymCoder, typename FreqCoder>
    inline void encode_histogram(BitOStream& out, SymCoder& sym_coder, FreqCoder& freq_coder) {
        size_t sigma = 0;
        for(size_t c = 0; c < MAX_SYMS; c++) {
            if(leaves_[c]) ++sigma;
        }

        out.write_delta(sigma);
        for(size_t c = 0; c < MAX_SYMS; c++) {
            Node* q = leaves_[c];
            if(q) {
                sym_coder.encode(out, q->sym);
                freq_coder.encode(out, q->weight);
            }
        }
    }

    template<typename SymCoder, typename FreqCoder>
    inline PriorityQueue decode_histogram(BitIStream& in, SymCoder& sym_coder, FreqCoder& freq_coder) {

        //decode histogram, create leaves and fill queue
        PriorityQueue queue(NodePriorityComparator{});

        const size_t sigma = in.read_delta<>();
        for(size_t i = 0; i < sigma; i++) {
            const Char c = sym_coder.template decode<Char>(in);
            const size_t w = freq_coder.template decode<>(in);

            Node* q = node(num_nodes_++);
            *q = Node { w, 0, nullptr, 0, nullptr, nullptr, c };
            leaves_[c] = q;

            queue.push(q);
        }
        return queue;
    }

    inline void replace(Node* u, Node* v) {
        u->parent = v->parent;
        u->rank = v->rank;
        u->bit = v->bit;

        if(u->parent) {
            if(u->bit) {
                u->parent->right = u;
            } else {
                u->parent->left = u;
            }
        }
    }

public:
    inline void encode(BitOStream& out, const Node* leaf) {
        assert(leaf);

        // encode
        std::vector<bool> bits; // TODO: static buffer
        for(const Node* v = leaf; v != root_; v = v->parent) {
            bits.push_back(v->bit);
        }

        for(size_t i = bits.size(); i > 0; i--) {
            out.write_bit(bits[i-1]);
        }
    }

    inline Char decode(BitIStream& in) {
        Node* v = root_;
        while(!v->is_leaf()) {
            v = in.read_bit() ? v->right : v->left;
        }
        return v->sym;
    }
};

}} // tdc::code
