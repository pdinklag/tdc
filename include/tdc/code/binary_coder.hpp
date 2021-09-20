#pragma once

#include <tdc/code/coder.hpp>

namespace tdc {
namespace code {

/// \brief Binary code.
template<size_t bits_ = 64ULL>
class BinaryCoder : public Coder {
public:
    /// \brief Default constructor.
    inline BinaryCoder() {
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
        out.write_binary(value, bits_);
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
        return in.template read_binary<T>(bits_);
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
