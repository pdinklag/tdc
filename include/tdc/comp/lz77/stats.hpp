#pragma once

namespace tdc {
namespace comp {
namespace lz77 {

/// \brief LZ77 compression statistics.
struct Stats {
    size_t input_size;
    size_t num_literals;
    size_t num_refs;
    
    size_t debug;
};

}}}
