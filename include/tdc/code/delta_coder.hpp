#pragma once

#include <tdc/code/coder.hpp>

namespace tdc {
namespace code {

/// \brief Elias delta code.
class DeltaCoder : public Coder {
public:
    using Coder::Coder;

    /// \brief Encodes an integer to the given output stream.
    /// \tparam T the integer type
    /// \param out the bit output stream to write to
    /// \param value the value to encode, must be greater than zero
    template<typename T>
    void encode(BitOStream& out, T value) {
        out.write_delta(value);
    }

    /// \brief Decodes an integer from the given input stream.
    /// \tparam T the integer type
    /// \param in the input stream to read from
    template<typename T = uint64_t>
    T decode(BitIStream& in) {
        return in.template read_delta<T>();
    }
};

}}
