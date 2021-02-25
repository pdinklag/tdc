#pragma once

#include <cassert>
#include <climits>
#include <concepts>
#include <cstdint>
#include <iostream>

#include <tdc/math/bit_mask.hpp>

namespace tdc {
namespace io {

/// \brief Wrapper for input streams that provides bitwise reading functionality.
///
/// Note that when used on a stream that has not been written using a \ref BitOStream, the end of the stream may not be detected correctly on bit level.
class BitIStream {
private:
    std::istream* m_stream;

    static constexpr uint8_t MSB = 7;

    uint8_t m_current = 0;
    uint8_t m_next = 0;

    bool m_is_final = false;
    uint8_t m_final_bits = 0;

    uint8_t m_cursor = 0;

    size_t m_bits_read = 0;

    void read_next_from_stream();
    void read_next();
    
public:
    /// \brief Constructs a bit input stream.
    /// \param stream the wrapped stream
    BitIStream(std::istream& stream);

    BitIStream(const BitIStream&) = delete;
    BitIStream(BitIStream&& other) = default;
    BitIStream& operator=(const BitIStream& other) = delete;
    BitIStream& operator=(BitIStream&& other) = default;

    /// \brief Tests whether the end of the input stream has been reached.
    ///iostream
    /// If the underlying stream was not written by a \ref BitOStream, the result may be incorrect and must be replaced by manual handling.
    inline bool eof() const {
        // If there are no more bytes, and all bits from the current buffer are read,
        // we are done
        return m_is_final && (m_cursor <= (MSB - m_final_bits));
    }

    /// \brief Reads a single bit from the stream.
    bool read_bit();

    /// \brief Reports the number of bits read from the stream.
    inline size_t bits_read() const { return m_bits_read; }

    /// \brief Decodes the binary encoding of an integer from the input stream.
    /// \tparam T the type of the value to read
    /// \param bits the number of bits to read; defaults to the byte-aligned size of the value type
    template<std::integral T = uint64_t>
    T read_binary(size_t bits = sizeof(T) * CHAR_BIT) {
        assert(bits <= 64ULL);

        const size_t bits_left_in_current = m_cursor + 1ULL;
        if(bits < bits_left_in_current) {
            // we are reading only few bits
            // simply use the bit-by-bit method
            T value = T(0);
            for(size_t i = 0; i < bits; i++) {
                value <<= T(1);
                value |= read_bit();
            }
            return value;
        } else {
            // we are at least consuming the current byte
            const size_t in_bits = bits;

            bits -= bits_left_in_current;
            uint64_t v = uint64_t(m_current) & math::bit_mask<uint64_t>(bits_left_in_current);
            v <<= bits;

            // read as many full bytes as possible
            if(bits >= 8ULL) {
                if(bits >= 16ULL) {
                    // use m_next and then read remaining bytes
                    const size_t n = (bits / 8ULL) - 1;
                    bits %= 8ULL;

                    const size_t off = sizeof(size_t) - n;

                    uint64_t v_bytes = 0;
                    m_stream->read(((char*)&v_bytes) + off, n);

                    // convert full bytes into LITTLE ENDIAN (!) representation
                    #if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
                        v_bytes = __builtin_bswap64(v_bytes);
                    #endif

                    v_bytes |= uint64_t(m_next) << (n * 8ULL);
                    v |= (v_bytes) << bits;

                    // keep data consistency
                    read_next_from_stream();
                } else {
                    // use m_next
                    bits -= 8ULL;
                    assert(!eof());
                    read_next();
                    v |= uint64_t(m_current) << bits;
                }
            }

            // get next byte
            read_next();

            // read remaining bits
            // they (must) be available in the next byte, so just mask them out
            if(bits) {
                assert(!eof());
                assert(bits < 8ULL);
                v |= (m_current >> (8ULL - bits));
                m_cursor = MSB - bits;
            }

            m_bits_read += in_bits;
            return v;
        }
    }

    /// \brief Decodes the unary encoding of an integer from the input stream.
    /// \tparam T the type of the value to read
    template<std::integral T = uint64_t>
    T read_unary() {
        T v = 0;
        while(!read_bit()) ++v;
        return v;
    }

    /// \brief Decodes the Elias gamma encoding of an integer from the input stream.
    /// \tparam T the type of the value to read
    template<std::integral T = uint64_t>
    T read_gamma() {
        auto m = read_unary<>();
        if(m > 0) {
            return T((1ULL << m) | read_binary<uint64_t>(m));
        } else {
            return T(1);
        }
    }
    
    /// \brief Decodes the Elias delta encoding of an integer from the input stream.
    /// \tparam T the type of the value to read
    template<std::integral T = uint64_t>
    T read_delta() {
        auto m = read_gamma<>() - 1;
        if(m > 0) {
            return T((1ULL << m) | read_binary<uint64_t>(m));
        } else {
            return T(1);
        }
    }

    /// \brief Decodes the Rice encoding of an integer from the input stream.
    /// \tparam T the type of the value to read
    /// \param p the exponent of the Golomb code divisor, which will be <tt>2^p</tt>
    template<std::integral T = uint64_t>
    T read_rice(uint8_t p) {
        const auto q = read_gamma<>() - 1;
        const auto r = read_binary<>(p);
        return T(q * (1ULL << p) + r);
    }
};

}} // namespace tdc::io
