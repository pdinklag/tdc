#pragma once

namespace tdc {
namespace comp {
namespace lz78 {

/// \brief LZ78 compression statistics.
struct Stats {
    size_t input_size;
    size_t trie_size;
    
    Stats()
        : input_size(0),
          trie_size(0)
    {}
};

}}}
