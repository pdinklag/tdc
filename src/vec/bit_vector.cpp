#include <tdc/vec/bit_vector.hpp>

using namespace tdc::vec;

std::unique_ptr<uint64_t[]> BitVector::allocate(const size_t num_bits) {
    const size_t num64 = div64_ceil(num_bits);
    uint64_t* p = new uint64_t[num64];
    memset(p, 0, num64 * sizeof(uint64_t));
    return std::unique_ptr<uint64_t[]>(p);
}
