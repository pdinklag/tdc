#include <tdc/vec/allocate.hpp>
#include <tdc/math/idiv.hpp>

std::unique_ptr<uint64_t[]> tdc::vec::allocate_integers(const size_t num, const size_t width, const bool initialize) {
    const size_t num64 = math::idiv_ceil(num * width, 64ULL);
    uint64_t* p = new uint64_t[num64];
    
    if(initialize) {
        memset(p, 0, num64 * sizeof(uint64_t));
    }
    
    return std::unique_ptr<uint64_t[]>(p);
}
