#include <tdc/malloc/malloc_callback.hpp>
#include <tdc/stat/phase.hpp>

namespace tdc {
namespace malloc_callback {
    void on_alloc(size_t bytes) {
        stat::Phase::track_mem_alloc(bytes);
    }
    
    void on_free(size_t bytes) {
        stat::Phase::track_mem_free(bytes);
    }
}}
