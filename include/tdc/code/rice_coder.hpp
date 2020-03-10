#pragma once

#include <tdc/code/coder.hpp>

namespace tdc {
namespace code {

/// \brief Rice code, a special variant of Golomb codes.
class RiceCoder : public Coder {
private:
    size_t m_golomb_exponent;

public:
    /// \brief Constructs a rice coder with the given parameters.
    /// \param p the exponent of the Golomb code divisor, which will be <tt>2^p</tt>.
    inline RiceCoder(const size_t p) : m_golomb_exponent(p) {
    }

    /// \brief Encodes an integer to the given output stream using \c m_bits.
    /// \tparam T the integer type
    /// \param out the bit output stream to write to
    /// \param value the value to encode
    template<typename T>
    void encode(BitOStream& out, T value) {
        out.write_rice(value, m_golomb_exponent);
    }

    /// \brief Decodes an integer from the given input stream using \c m_bits.
    /// \tparam T the integer type
    /// \param in the input stream to read from
    template<typename T = uint64_t>
    T decode(BitIStream& in) {
        return in.template read_rice<T>(m_golomb_exponent);
    }
};

}} // namespace tdc::code
