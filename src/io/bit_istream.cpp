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
