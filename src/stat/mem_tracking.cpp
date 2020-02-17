#include <tdc/stat/malloc_callback.hpp>
#include <tdc/stat/phase.hpp>

namespace tdc {
namespace stat {
    void on_alloc(size_t bytes) {
        Phase::track_mem_alloc(bytes);
    }
    
    void on_free(size_t bytes) {
        Phase::track_mem_free(bytes);
    }
}}
