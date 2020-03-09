#include <tdc/vec/bit_vector.hpp>

using namespace tdc::vec;

BitVector::BitVector(const std::vector<bool>& bits) : m_size(bits.size()) {
    m_bits = allocate_integers(m_size, 1, false);
    
    // TODO: is there a faster way?
    for(size_t i = 0; i < m_size; i++) {
        set(i, bits[i]);
    }
}

void BitVector::resize(const size_t size) {
    BitVector new_bv(size, size >= m_size); // no initialization needed if new size is smaller

    const size_t num_to_copy = std::min(size, m_size);

    // copy block-wise while possible
    const size_t num_blocks64 = num_to_copy / 64ULL;
    for(size_t i = 0; i < num_blocks64; i++) {
        new_bv.m_bits[i] = m_bits[i];
    }

    // copy remaining bits one by one
    for(size_t i = num_blocks64 * 64ULL; i < num_to_copy; i++) {
        new_bv.set(i, get(i));
    }
    
    *this = std::move(new_bv);
}
