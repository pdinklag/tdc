#pragma once

#include <utility>
#include <vector>

#include <tdc/code/huff/adaptive_huffman_coder_base.hpp>

namespace tdc {
namespace code {

// dynamic (online) Huffman coding according to [Knuth, 1985]
template<typename SymCoder>
class KnuthCoder : public AdaptiveHuffmanCoderBase {
private:
    SymCoder sym_coder_;

public:
    inline KnuthCoder() : AdaptiveHuffmanCoderBase() {
        // init tree with NYT node
        num_nodes_ = 1;
        nodes_[0] = Node{ 0, 0, nullptr, 0, nullptr, nullptr, 0 };
        root_ = node(0);

        // initialize symbol index
        for(size_t c = 0; c < MAX_SYMS; c++) {
            leaves_[c] = nullptr;
        }
    }

    inline KnuthCoder(const std::string& s, BitOStream& out) : KnuthCoder() {
    }

    inline KnuthCoder(BitIStream& in) : KnuthCoder() {
    }

    inline void encode(BitOStream& out, const Char c) {
        const Node* q = leaves_[c];
        bool is_nyt;
        if(q) {
            is_nyt = false;
        } else {
            q = node(0);
            is_nyt = true;
        }

        // encode symbol unless it is the first
        if(!(is_nyt && num_nodes_ == 1)) {
            std::vector<bool> bits; // TODO: use static buffer
            for(const Node* v = q; v != root_; v = v->parent) {
                bits.push_back(v->bit);
            }

            for(size_t i = bits.size(); i > 0; i--) {
                out.write_bit(bits[i-1]);
            }
        }

        // if symbol is NYT, encode ASCII encoding
        if(is_nyt) {
            sym_coder_.encode(out, c);
        }

        // increase weight
        increase(c);
    }

    inline Char decode(BitIStream& in) {
        Char c;
        if(num_nodes_ == 1) {
            // first character
            c = sym_coder_.template decode<Char>(in);
        } else {        
            Node* v = root_;
            while(!v->is_leaf()) {
                v = in.read_bit() ? v->right : v->left;
            }

            if(v == node(0)) {
                // NYT
                c = sym_coder_.template decode<Char>(in);
            } else {
                c = v->sym;
            }
        }

        // increase weight
        increase(c);
        return c;
    }

private:
    inline void increase(const Char c) {
        // find or create leaf for c
        Node* nyt = node(0);
        
        Node* q = leaves_[c];
        if(!q) {
            // new symbol - we need to add a leaf for it in the tree

            // since we will add two nodes (leaf and inner node)
            // with zero weight, they will be rank 1 and 2, respectively
            // therefore, increase the ranks of all current nodes by two
            // (except NYT, which remains 0)
            for(size_t i = 1; i < num_nodes_; i++) {
                nodes_[i].rank += 2;
            }
            
            // create new inner node v
            Node* v = node(num_nodes_++);

            // create new leaf q
            q = node(num_nodes_++);
            leaves_[c] = q;

            // initialize v by replacing NYT
            *v = Node { 0, 2, nyt->parent, nyt->bit, nyt, q, 0 };
            if(nyt->parent) nyt->parent->left = v;

            // make NYT left child of v
            nyt->parent = v;
            nyt->bit = 0; // TODO: is this necessary?

            // if root is still NYT, v becomes the root and stays root forever
            if(root_ == nyt) root_ = v;

            // initialize q as right child of v
            *q = Node { 0, 1, v, 1, nullptr, nullptr, c };
        }

        // if q is the sibling of NYT, we need to move it elsewhere
        // by the structure of ranks, q is the sibling of NYT
        // if and only if its rank is 1
        if(q->rank == 1) {
            // identify the leaf of highest rank with the same weight as q
            // denote it as u
            const size_t w = q->weight;
            Node* u = q;
            for(size_t rank = q->rank + 1; rank < MAX_SYMS; rank++) {
                Node* x = rank_map_[rank];
                if(x) {
                    if(x->is_leaf() && x->weight == w) {
                        u = x;
                    } else if( x->weight > w) {
                        break;
                    }
                }
            }

            if(u != q) {
                // interchange q and u
                Node temp_u = *u;
                replace(u, q);
                replace(q, &temp_u);
            }

            ++q->weight;
            q = q->parent;
        }

        // we now move up the tree, starting with current node v = q
        // and restructure the tree so incrementing the frequency becomes easy
        // [Vitter 1985] calls this the "first phase"
        for(Node* v = q; v; v = v->parent) {
            // first, we identify the node of highest rank with the same weight
            // we denote that node as u
            // NYT can always be skipped
            const size_t w = v->weight;
            Node* u = v;
            for(size_t rank = v->rank + 1; rank < MAX_SYMS; rank++) {
                Node* x = rank_map_[rank];
                if(x) {
                    if(x->weight == w) {
                        u = x;
                    } else if(x->weight > w) {
                        break;
                    }
                }
            }

            if(u != v) {
                // interchange u and v
                Node temp_u = *u;
                replace(u, v);
                replace(v, &temp_u);
            }
        }

        // now we simply increment the weights up the tree from q
        for(; q; q = q->parent) ++q->weight;
    }
};

}} // namespace tdc::code
