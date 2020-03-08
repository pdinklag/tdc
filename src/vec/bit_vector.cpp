#include <tdc/vec/bit_vector.hpp>

using namespace tdc::vec;

BitVector::BitVector(const std::vector<bool>& bits) : m_size(bits.size()) {
    m_bits = allocate_integers(m_size, 1, false);
    
    // TODO: is there a faster way?
    for(size_t i = 0; i < m_size; i++) {
        set(i, bits[i]);
    }
}
