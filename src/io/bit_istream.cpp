#include <tdc/io/bit_istream.hpp>

using namespace tdc::io;

void BitIStream::read_next_from_stream() {
    char c;
    if(m_stream->get(c)) {
        m_next = c;

        if(m_stream->get(c)) {
            // stream still going
            m_stream->unget();
        } else {
            // stream over after next, do some checks
            m_final_bits = c;
            m_final_bits &= 0b111;
            if(m_final_bits >= 6) {
                // special case - already final
                m_is_final = true;
                m_next = 0;
            }
        }
    } else {
        m_is_final = true;
        m_final_bits = m_current & 0b111;

        m_next = 0;
    }
}

void BitIStream::read_next() {
    m_current = m_next;
    m_cursor = MSB;

    read_next_from_stream();
}

BitIStream::BitIStream(std::istream& input) : m_stream(&input) {
    char c;
    if(m_stream->get(c)) {
        // prepare the state by reading the first byte into to the `m_next`
        // member. `read_next()` will then shift it to the `m_current`
        // member, from which the `read_XXX()` methods take away bits.
        m_is_final = false;
        m_next = c;

        read_next();
    } else {
        // special case: if the stream is empty to begin with, we
        // never read the last 3 bits and just treat it as completely empty
        m_is_final = true;
        m_final_bits = 0;
    }
}

bool BitIStream::read_bit() {
    if(!eof()) {
        const bool bit = (m_current >> m_cursor) & 1;
        if(m_cursor) {
            --m_cursor;
        } else {
            read_next();
        }

        ++m_bits_read;
        return bit;
    } else {
        return 0; //EOF
    }
}

template uint8_t BitIStream::read_binary<uint8_t>(size_t);
template uint16_t BitIStream::read_binary<uint16_t>(size_t);
template uint32_t BitIStream::read_binary<uint32_t>(size_t);
template uint64_t BitIStream::read_binary<uint64_t>(size_t);

template uint8_t BitIStream::read_unary<uint8_t>();
template uint16_t BitIStream::read_unary<uint16_t>();
template uint32_t BitIStream::read_unary<uint32_t>();
template uint64_t BitIStream::read_unary<uint64_t>();

template uint8_t BitIStream::read_gamma<uint8_t>();
template uint16_t BitIStream::read_gamma<uint16_t>();
template uint32_t BitIStream::read_gamma<uint32_t>();
template uint64_t BitIStream::read_gamma<uint64_t>();

template uint8_t BitIStream::read_delta<uint8_t>();
template uint16_t BitIStream::read_delta<uint16_t>();
template uint32_t BitIStream::read_delta<uint32_t>();
template uint64_t BitIStream::read_delta<uint64_t>();

template uint8_t BitIStream::read_rice<uint8_t>(uint8_t);
template uint16_t BitIStream::read_rice<uint16_t>(uint8_t);
template uint32_t BitIStream::read_rice<uint32_t>(uint8_t);
template uint64_t BitIStream::read_rice<uint64_t>(uint8_t);
