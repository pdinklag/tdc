#pragma once

#include <tdc/code/coder.hpp>

namespace tdc {
namespace code {

/// \brief Binary code.
class BinaryCoder : public Coder {
private:
    size_t m_bits;

public:
    /// \brief Default constructor, defaulting the coder to 64 bits per integer.
    inline BinaryCoder() : BinaryCoder(64ULL) {
    }

    /// \brief Constructs a binary coder with a given default bit count.
    /// \param bits the default number of bits per integer
    inline BinaryCoder(const size_t bits) : m_bits(bits) {
    }

    BinaryCoder(const BinaryCoder& other) = default;
    BinaryCoder(BinaryCoder&& other) = default;
    BinaryCoder& operator=(const BinaryCoder& other) = default;
    BinaryCoder& operator=(BinaryCoder&& other) = default;

    /// \brief Encodes an integer to the given output stream using \c m_bits.
    /// \tparam T the integer type
    /// \param out the bit output stream to write to
    /// \param value the value to encode
    template<typename T>
    void encode(BitOStream& out, T value) {
        out.write_binary(value, m_bits);
    }

    /// \brief Encodes an integer to the given output stream using the specified number of bits.
    /// \tparam T the integer type
    /// \param out the bit output stream to write to
    /// \param value the value to encode
    /// \param bits the number of bits of the integer code
    template<typename T>
    void encode(BitOStream& out, T value, const size_t bits) {
        out.write_binary(value, bits);
    }

    /// \brief Decodes an integer from the given input stream using \c m_bits.
    /// \tparam T the integer type
    /// \param in the input stream to read from
    template<typename T = uint64_t>
    T decode(BitIStream& in) {
        return in.template read_binary<T>(m_bits);
    }

    /// \brief Decodes an integer from the given input stream using the specified number of bits.
    /// \tparam T the integer type
    /// \param in the input stream to read from
    /// \param bits the number of bits of the integer code
    template<typename T = uint64_t>
    T decode(BitIStream& in, const size_t bits) {
        return in.template read_binary<T>(bits);
    }
};

}} // namespace tdc::code
