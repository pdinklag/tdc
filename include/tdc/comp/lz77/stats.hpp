#pragma once

namespace tdc {
namespace comp {
namespace lz77 {

/// \brief LZ77 compression statistics.
struct Stats {
    size_t input_size;
    size_t num_literals;
    size_t num_refs;
    
    size_t num_collisions;
    size_t num_extensions;
    size_t num_swaps;
    size_t extension_sum;
    size_t trie_size;
    
    Stats()
        : input_size(0),
          num_literals(0),
          num_refs(0),
          num_collisions(0),
          num_extensions(0),
          num_swaps(0),
          extension_sum(0),
          trie_size(0)
    {}
};

}}}
