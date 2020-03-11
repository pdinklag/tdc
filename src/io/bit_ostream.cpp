#include <tdc/io/bit_ostream.hpp>
#include <tdc/util/uint40.hpp>

using namespace tdc::io;
using tdc::uint40_t;

void BitOStream::reset() {
    m_next = 0;
    m_cursor = MSB;
}

void BitOStream::write_next() {
    m_stream->put(char(m_next));
    reset();
}

BitOStream::BitOStream(std::ostream& stream) : m_stream(&stream) {
    reset();
}

BitOStream::~BitOStream() {
    uint8_t set_bits = MSB - m_cursor; // will only be in range 0 to 7
    if(m_cursor >= 2) {
        // if there are at least 3 bits free in the byte buffer,
        // write them into the cursor at the last 3 bit positions
        m_next |= set_bits;
        write_next();
    } else {
        // else write out the byte, and write the length into the
        // last 3 bit positions of the next byte
        write_next();
        m_next = set_bits;
        write_next();
    }
}

void BitOStream::write_bit(const bool b) {
    m_next |= (b << m_cursor);

    --m_cursor;
    if(m_cursor < 0) {
        write_next();
    }

    ++m_bits_written;
}

template void BitOStream::write_binary<uint8_t>(const uint8_t, size_t bits);
template void BitOStream::write_binary<uint16_t>(const uint16_t, size_t bits);
template void BitOStream::write_binary<uint32_t>(const uint32_t, size_t bits);
template void BitOStream::write_binary<uint40_t>(const uint40_t, size_t bits);
template void BitOStream::write_binary<uint64_t>(const uint64_t, size_t bits);

template void BitOStream::write_unary<uint8_t>(const uint8_t);
template void BitOStream::write_unary<uint16_t>(const uint16_t);
template void BitOStream::write_unary<uint32_t>(const uint32_t);
template void BitOStream::write_unary<uint40_t>(const uint40_t);
template void BitOStream::write_unary<uint64_t>(const uint64_t);

template void BitOStream::write_gamma<uint8_t>(const uint8_t);
template void BitOStream::write_gamma<uint16_t>(const uint16_t);
template void BitOStream::write_gamma<uint32_t>(const uint32_t);
template void BitOStream::write_gamma<uint40_t>(const uint40_t);
template void BitOStream::write_gamma<uint64_t>(const uint64_t);

template void BitOStream::write_delta<uint8_t>(const uint8_t);
template void BitOStream::write_delta<uint16_t>(const uint16_t);
template void BitOStream::write_delta<uint32_t>(const uint32_t);
template void BitOStream::write_delta<uint40_t>(const uint40_t);
template void BitOStream::write_delta<uint64_t>(const uint64_t);

template void BitOStream::write_rice<uint8_t>(const uint8_t, const uint8_t);
template void BitOStream::write_rice<uint16_t>(const uint16_t, const uint8_t);
template void BitOStream::write_rice<uint32_t>(const uint32_t, const uint8_t);
template void BitOStream::write_rice<uint40_t>(const uint40_t, const uint8_t);
template void BitOStream::write_rice<uint64_t>(const uint64_t, const uint8_t);
