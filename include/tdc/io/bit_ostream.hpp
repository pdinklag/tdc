#pragma once

#include <cassert>
#include <climits>
#include <iostream>
#include <utility>

#include <tdc/math/bit_mask.hpp>
#include <tdc/math/ilog2.hpp>

namespace tdc {
namespace io {

/// \brief Wrapper for output streams that provides bitwise writing functionality.
///
/// Bits are written into an internal buffer byte which is written to the output when it is either filled or when the wrapper is destroyed.
class BitOStream {
private:
    std::ostream* m_stream;

    uint8_t m_next;
    int8_t m_cursor;
    static constexpr int8_t MSB = 7;

    size_t m_bits_written = 0;

    inline bool is_dirty() const {
        return m_cursor != MSB;
    }

    void reset();
    void write_next();

public:
    /// \brief Constructs a bit output stream.
    /// \param stream the wrapped stream
    BitOStream(std::ostream& stream);

    /// \brief Finalizes the bit output stream by writing a lead-out.
    ///
    /// Note that the destructor has important functionality. For a \ref BitIStream to know how many bits are available in the last byte,
    /// the destructor writes at most two additional lead-out bytes which contain that information.
    ~BitOStream();

    BitOStream(const BitOStream&) = delete;
    BitOStream(BitOStream&& other) = default;
    BitOStream& operator=(const BitOStream& other) = delete;
    BitOStream& operator=(BitOStream&& other) = default;

    /// \brief Gets the underlying output stream.
    inline std::ostream& stream() {
        return *m_stream;
    }

    /// \brief Reports the number of bits written on the stream.
    inline size_t bits_written() const {
        return m_bits_written;
    }

    /// \brief Writes a single bit to the output stream.
    /// \param b the bit to write
    void write_bit(bool b);

    /// \brief Writes the binary encoding of an integer to the output stream.
    /// \tparam T the type of the value to write
    /// \param value the value to write
    /// \param bits the number of bits to write; defaults to the byte-aligned size of the value type
    template<typename T>
    void write_binary(const T value, size_t bits = sizeof(T) * CHAR_BIT) {
        assert(bits <= 64ULL);
        assert(m_cursor >= 0);
        const size_t bits_left_in_next = size_t(m_cursor + 1);
        assert(bits_left_in_next <= 8ULL);

        if(bits < bits_left_in_next) {
            // we are writing only few bits
            // simply use the bit-by-bit method
            for (int i = bits - 1; i >= 0; i--) {
                write_bit((value & T(1ULL << i)) != T(0));
            }
        } else {
            // we are at least finishing the next byte
            const size_t in_bits = bits;

            // mask low bits of value
            uint64_t v = uint64_t(value) & math::bit_mask<uint64_t>(bits);

            // fill it up next byte and continue with remaining bits
            bits -= bits_left_in_next;
            m_next |= (v >> bits);
            write_next();

            v &= math::bit_mask<uint64_t>(bits); // mask remaining bits

            // write as many full bytes as possible
            if(bits >= 8ULL) {
                const size_t n = bits / 8ULL;
                bits %= 8ULL;

                // convert full bytes into BIG ENDIAN (!) representation
                #if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
                    const size_t v_bytes = __builtin_bswap64(v >> bits);
                #else
                    const size_t v_bytes = v >> bits;
                #endif

                const size_t off  = sizeof(size_t) - n;
                m_stream->write(((const char*)&v_bytes) + off, n);

                v &= math::bit_mask<uint64_t>(bits); // mask remaining bits
            }

            if(bits) {
                // write remaining bits
                // they (must) fit into the next byte, so just stuff them in
                assert(bits < 8ULL);
                assert(v < 256ULL);
                m_next = (v << (8ULL - bits));
                m_cursor = MSB - int8_t(bits);
                assert(m_cursor >= 0);
            }

            m_bits_written += in_bits;
        }
    }

    /// \brief Writes the unary encoding of an integer to the output stream.
    ///
    /// For an integer \c x, this will write \c x 0-bits followed by a 1-bit.
    ///
    /// \tparam T the type of the value to write
    /// \param value the value to write
    template<typename T>
    void write_unary(T value) {
        while(value--) {
            write_bit(0);
        }
        write_bit(1);
    }

    /// \brief Writes the Elias gamma code for an integer to the output stream.
    ///
    /// \tparam T the type of the value to write
    /// \param value the value to write, must be greater than zero
    template<typename T>
    void write_gamma(T value) {
        assert(value > T(0));

        const auto m = math::ilog2_floor(value);
        write_unary(m);
        if(m > 0) write_binary(value, m); // cut off leading 1
    }

    /// \brief Writes the Elias delta code for an integer to the output stream.
    ///
    /// \tparam T the type of the value to write
    /// \param value the value to write, must be greater than zero
    template<typename T>
    void write_delta(T value) {
        assert(value > T(0));

        auto m = math::ilog2_floor(value);
        write_gamma(m+1); // cannot encode zero
        if(m > 0) write_binary(value, m); // cut off leading 1
    }


    /// \brief Writes the Rice code for an integer to the output stream.
    ///
    /// \tparam T the type of the value to write
    /// \param value the value to write
    /// \param p the exponent of the Golomb code divisor, which will be <tt>2^p</tt>
    template<typename T>
    void write_rice(T value, const uint8_t p) {
        const uint64_t q = uint64_t(value) >> p;

        write_gamma(q + 1);
        write_binary(value, p); // r is exactly the lowest p bits of v
    }
};

}} // namespace tdc::io
