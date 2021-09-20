#pragma once

#include <utility>
#include <vector>

#include <tdc/code/huff/huffman_coder_base.hpp>

namespace tdc {
namespace code {

// forward dynamic Huffman coding according to [Klein et al., 2019]
template<typename SymCoder, typename FreqCoder>
class ForwardCoder : public AdaptiveHuffmanCoderBase {
private:
    SymCoder  sym_coder_;
    FreqCoder freq_coder_;

    inline ForwardCoder() : AdaptiveHuffmanCoderBase() {
        // initialize
        for(size_t c = 0; c < MAX_SYMS; c++) {
            leaves_[c] = nullptr;
        }
        num_nodes_ = 0;
    }

public:
    inline ForwardCoder(const std::string& s, BitOStream& out) : ForwardCoder() {
        // build Huffman tree and write histogram
        auto queue = init_leaves(s);
        build_tree(queue);
        encode_histogram(out, sym_coder_, freq_coder_);
    }

    inline ForwardCoder(BitIStream& in) : ForwardCoder() {
        // read histogram and build Huffman tree
        auto queue = decode_histogram(in, sym_coder_, freq_coder_);
        build_tree(queue);
    }

    inline bool decode_eof(BitIStream& in) const {
        if(in.eof()) {
            return !root_;
        } else {
            return false;
        }
    }

    inline void encode(BitOStream& out, const Char c) {
        if(root_->is_leaf()) {
            // only root is left, and it's unique, no need to encode
        } else {
            // default
            HuffmanCoderBase::encode(out, leaves_[c]);
        }

        // decrease weight
        decrease(c);
    }

    inline Char decode(BitIStream& in) {
        Char c;
        if(root_->is_leaf()) {
            // only root is left, report its symbol
            c = root_->sym;
        } else {
            // default
            c = HuffmanCoderBase::decode(in);
        }

        // decrease weight
        decrease(c);
        return c;
    }

private:
    inline void decrease(Char c) {
        assert(leaves_[c]);
        
        for(Node* p = leaves_[c]; p; p = p->parent) {
            // find lowest ranked node q with same weight as p
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
                // interchange p and q
                Node temp_q = *q;
                replace(q, p);
                replace(p, &temp_q);
            }

            --p->weight;
        }

        Node* p = leaves_[c];
        if(p->weight == 0) {
            // last occurrence of c
            if(p == root_) {
                // last symbol
                leaves_[c] = nullptr;
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
                leaves_[c] = nullptr;

                Node* sibling = p->bit ? parent->left : parent->right;

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
                    root_ = sibling;
                }
            }
        }
    }
};

}} // namespace tdc::code
