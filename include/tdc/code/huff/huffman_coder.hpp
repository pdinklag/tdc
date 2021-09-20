#pragma once

#include <vector>
#include <tdc/code/huff/huffman_coder_base.hpp>

namespace tdc {
namespace code {

// Huffman coder based on [Huffman, 1952]
template<typename SymCoder, typename FreqCoder>
class HuffmanCoder : public HuffmanCoderBase {
private:
    SymCoder  sym_coder_;
    FreqCoder freq_coder_;

    inline HuffmanCoder() : HuffmanCoderBase() {
        // initialize
        for(size_t c = 0; c < MAX_SYMS; c++) {
            leaves_[c] = nullptr;
        }
        num_nodes_ = 0;
    }

    // assumes that queue contains all the leaves!
    inline void build_tree(PriorityQueue& queue) {
        assert(!queue.empty());
        
        const size_t sigma = queue.size();
        for(size_t i = 0; i < sigma - 1; i++) {
            // get the next two nodes from the priority queue
            Node* l = queue.top(); queue.pop();
            Node* r = queue.top(); queue.pop();

            // make sure l has the smaller weight
            if(r->weight < l->weight) {
                std::swap(l, r);
            }

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
        assert(queue.empty());
    }

public:
    inline HuffmanCoder(const std::string& s, BitOStream& out) : HuffmanCoder() {
        // build Huffman tree and write histogram
        auto queue = init_leaves(s);
        build_tree(queue);
        encode_histogram(out, sym_coder_, freq_coder_);
    }

    inline HuffmanCoder(BitIStream& in) : HuffmanCoder() {
        // read histogram and build Huffman tree
        auto queue = decode_histogram(in, sym_coder_, freq_coder_);
        build_tree(queue);
    }

    inline void encode(BitOStream& out, const Char c) {
        HuffmanCoderBase::encode(out, leaves_[c]);
    }
};

}} // namespace tdc::code
