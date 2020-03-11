#include <tdc/code/binary_coder.hpp>
#include <tdc/code/delta_coder.hpp>
#include <tdc/code/delta0_coder.hpp>
#include <tdc/code/rice_coder.hpp>

#include <tdc/util/uint40.hpp>

using namespace tdc::code;
using tdc::uint40_t;

#define INSTANTIATE_DEFAULT(type) \
    template void type::encode<uint8_t>(BitOStream&, uint8_t); \
    template void type::encode<uint16_t>(BitOStream&, uint16_t); \
    template void type::encode<uint32_t>(BitOStream&, uint32_t); \
    template void type::encode<uint40_t>(BitOStream&, uint40_t); \
    template void type::encode<uint64_t>(BitOStream&, uint64_t); \
    template uint8_t  type::decode<uint8_t>(BitIStream&); \
    template uint16_t type::decode<uint16_t>(BitIStream&); \
    template uint32_t type::decode<uint32_t>(BitIStream&); \
    template uint40_t type::decode<uint40_t>(BitIStream&); \
    template uint64_t type::decode<uint64_t>(BitIStream&);

// default
INSTANTIATE_DEFAULT(BinaryCoder)
INSTANTIATE_DEFAULT(DeltaCoder)
INSTANTIATE_DEFAULT(Delta0Coder)
INSTANTIATE_DEFAULT(RiceCoder)

// special overloads for BinaryCoder
template void BinaryCoder::encode<uint8_t>(BitOStream&, uint8_t, const size_t);
template void BinaryCoder::encode<uint16_t>(BitOStream&, uint16_t, const size_t);
template void BinaryCoder::encode<uint32_t>(BitOStream&, uint32_t, const size_t);
template void BinaryCoder::encode<uint40_t>(BitOStream&, uint40_t, const size_t);
template void BinaryCoder::encode<uint64_t>(BitOStream&, uint64_t, const size_t);

template uint8_t  BinaryCoder::decode<uint8_t>(BitIStream&, const size_t);
template uint16_t BinaryCoder::decode<uint16_t>(BitIStream&, const size_t);
template uint32_t BinaryCoder::decode<uint32_t>(BitIStream&, const size_t);
template uint40_t BinaryCoder::decode<uint40_t>(BitIStream&, const size_t);
template uint64_t BinaryCoder::decode<uint64_t>(BitIStream&, const size_t);
