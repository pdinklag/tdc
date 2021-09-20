#pragma once

#include <utility>
#include <vector>

#include <tdc/code/huff/adaptive_huffman_coder_base.hpp>

namespace tdc {
namespace code {

// forward dynamic Huffman coding according to [Fruchtman et al., 2019]
template<typename SymCoder, typename FreqCoder>
class HybridForwardCoder : public AdaptiveHuffmanCoderBase {
private:
    SymCoder  sym_coder_;
    FreqCoder freq_coder_;

    index_t hist_[MAX_SYMS];

    inline HybridForwardCoder() : AdaptiveHuffmanCoderBase() {
        // initialize
        for(size_t c = 0; c < MAX_SYMS; c++) {
            leaves_[c] = nullptr;
            hist_[c] = 0;
        }
        num_nodes_ = 0;
    }

public:
    inline HybridForwardCoder(const std::string& s, BitOStream& out) : HybridForwardCoder() {
        // count histogram
        size_t sigma = 0;
        for(Char c : s) {
            if(!hist_[c]) ++sigma;
            ++hist_[c];
        }

        // init tree with NYT node
        num_nodes_ = 1;
        nodes_[0] = Node{ sigma, 0, nullptr, 0, nullptr, nullptr, 0 };
        root_ = node(0);

        // write sigma
        freq_coder_.encode(out, sigma);
    }

    inline HybridForwardCoder(BitIStream& in) : HybridForwardCoder() {
        // read sigma
        const size_t sigma = freq_coder_.template decode<>(in);

        // init tree with NYT node
        num_nodes_ = 1;
        nodes_[0] = Node{ sigma, 0, nullptr, 0, nullptr, nullptr, 0 };
        root_ = node(0);
    }

    inline bool decode_eof(BitIStream& in) const {
        if(in.eof()) {
            return !root_;
        } else {
            return false;
        }
    }

    inline void encode(BitOStream& out, const Char c) {
        const Node* q = leaves_[c];
        if(q == root_) {
            // only symbol in existance, no need to encode
        } else {
            // mostly like Knuth85
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

            // if symbol is NYT, encode ASCII encoding AND frequency
            if(is_nyt) {
                sym_coder_.encode(out, c);
                freq_coder_.encode(out, hist_[c]);
            }
        }

        // update
        update(c);
    }

    inline Char decode(BitIStream& in) {
        Char c;
        if(in.eof()) {
            // last symbol
            assert(root_ && root_->is_leaf());
            c = root_->sym;
        } else {
            // nearly the same as Knuth85
            if(num_nodes_ == 1) {
                // first character
                c = sym_coder_.template decode<Char>(in);
                hist_[c] = freq_coder_.template decode<>(in);
            } else {
                Node* v = root_;
                while(!v->is_leaf()) {
                    v = in.read_bit() ? v->right : v->left;
                }

                if(v == node(0)) {
                    // NYT
                    c = sym_coder_.template decode<Char>(in);
                    hist_[c] = freq_coder_.template decode<>(in);
                } else {
                    c = v->sym;
                }
            }
        }

        // update
        update(c);
        return c;
    }

private:
    inline void decrease(Node* leaf) {
        assert(leaf->is_leaf());

        // practically the same as Forward::update
        for(Node* p = leaf; p; p = p->parent) {
            // find lowest ranked node with same weight as p
            Node* q = p;
            const size_t w = p->weight;

            for(size_t rank = p->rank; rank > 0; rank--) {
                Node* x = rank_map_[rank-1];
                if(x) {
                    if(x->weight == w) {
                        q = x;
                    } else if(x->weight < w) {
                        break;
                    }
                }
            }

            if(p != q) {
                assert(q != root_);

                // interchange p and q
                Node temp_q = *q;
                replace(q, p);
                replace(p, &temp_q);
            }

            assert(p->weight > 0);
            --p->weight;
        }

        Node* p = leaf;
        if(p->weight == 0) {
            // last occurrence of the character
            if(p == root_) {
                // last symbol
                leaves_[root_->sym] = nullptr;
                root_ = nullptr;
            } else {
                // must have a parent, otherwise it's the root already
                Node* parent = p->parent;
                assert(parent);

                // "delete" p and its parent by setting infinite weights
                p->weight = SIZE_MAX;
                rank_map_[p->rank] = nullptr;
                parent->weight = SIZE_MAX;
                rank_map_[parent->rank] = nullptr;

                if(p != node(0)) {
                    leaves_[p->sym] = nullptr;
                }

                Node* sibling = p->bit ? parent->left : parent->right;
                assert(sibling);

                sibling->parent = parent->parent;
                sibling->bit = parent->bit;
                if(sibling->parent) {
                    if(sibling->bit) {
                        sibling->parent->right = sibling;
                    } else {
                        sibling->parent->left = sibling;
                    }
                } else {
                    assert(root_ == parent);
                    assert(!sibling->parent);
                    root_ = sibling;
                }
            }
        }
    }

    inline void insert(Char c) {
        const size_t w = hist_[c] - 1;

        // only insert if it's going to occur again
        if(w > 0) {
            // find the highest ranked node v with weight <= w
            // or, if it does not exist, a node with minimum weight
            Node* v = nullptr;
            Node* min = nullptr;
            for(size_t rank = 0; rank < MAX_SYMS; rank++) {
                Node* x = rank_map_[rank];
                if(x) {
                    if(!min) min = x;
                    if(x->weight > w) {
                        break;
                    } else {
                        v = x;
                    }
                }
            }

            for(size_t i = 0; i < num_nodes_; i++) {
                Node* x = node(i);
                if((!v || x->rank > v->rank) && x->weight <= w) {
                    v = x;
                }

                if(!min || x->weight < min->weight) min = x;
            }

            if(!v) v = min;
            assert(v); // there must be at least NYT

            // increase ranks of nodes with rank > v->rank by two
            // (we will insert nodes with ranks v->rank+1 and v->rank+2)
            for(size_t i = 0; i < num_nodes_; i++) {
                Node* x = node(i);
                if(x->rank > v->rank) set_rank(x, x->rank + 2, false);
                assert(x->rank < MAX_NODES);
            }

            // create a new leaf q for c
            Node* q = node(num_nodes_++);
            leaves_[c] = q;

            // create a new inner node u at the place of v
            // with left child v (so ranks remain) and right child the new leaf
            Node* u = node(num_nodes_++);
            *u = Node { v->weight+w, v->rank+2, v->parent, v->bit, v, q, 0 };

            // properly assign rank
            assert(u->rank < MAX_NODES);
            assert(rank_map_[u->rank] == nullptr);
            set_rank(u, u->rank, true);

            if(v->parent) {
                if(v->bit) {
                    v->parent->right = u;
                } else {
                    v->parent->left = u;
                }
            }
            v->parent = u;
            v->bit = 0;

            if(v == root_) {
                assert(!u->parent);
                root_ = u;
            }

            // init leaf
            *q = Node { w, v->rank+1, u, 1, nullptr, nullptr, c };

            // properly assign rank
            assert(q->rank < MAX_NODES);
            assert(rank_map_[q->rank] == nullptr);
            set_rank(q, q->rank, true);

            // increase weights up
            for(v = u->parent; v; v = v->parent) {
                v->weight += w;
            }
        }
    }

    inline void update(Char c) {
        if(leaves_[c]) {
            decrease(leaves_[c]);
        } else {
            insert(c);
            decrease(node(0)); // consume a NYT
        }
    }
};

}}
