#include <tdc/vec/sorted_sequence.hpp>

using namespace tdc::vec;

size_t SortedSequence::encode_unary(size_t pos, uint64_t value) {
    auto& bits = *m_bits;
    while(value) {
        bits[pos++] = 1;
        --value;
    }
    bits[pos++] = 0;
    return pos;
}

template SortedSequence::SortedSequence<const uint8_t*>(const uint8_t* const&, const size_t);
template SortedSequence::SortedSequence<const uint16_t*>(const uint16_t* const&, const size_t);
template SortedSequence::SortedSequence<const uint32_t*>(const uint32_t* const&, const size_t);
template SortedSequence::SortedSequence<const uint64_t*>(const uint64_t* const&, const size_t);

template SortedSequence::SortedSequence<uint8_t*>(uint8_t* const&, const size_t);
template SortedSequence::SortedSequence<uint16_t*>(uint16_t* const&, const size_t);
template SortedSequence::SortedSequence<uint32_t*>(uint32_t* const&, const size_t);
template SortedSequence::SortedSequence<uint64_t*>(uint64_t* const&, const size_t);
