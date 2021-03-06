#pragma once

#include <tdc/code/coder.hpp>

namespace tdc {
namespace code {

/// \brief Elias delta code with support for writing zero.
///
/// Instead of writing the delta code for an integer \c x, it writes the delta code for <tt>x+1</tt>.
class Delta0Coder : public Coder {
public:
    using Coder::Coder;

    /// \brief Encodes an integer to the given output stream.
    /// \tparam T the integer type
    /// \param out the bit output stream to write to
    /// \param value the value to encode, must be less than <tt>UINT64_MAX-1</tt>
    template<typename T>
    void encode(BitOStream& out, T value) {
        ++value;
        out.write_delta(value);
    }
    
    /// \brief Decodes an integer from the given input stream.
    /// \tparam T the integer type
    /// \param in the input stream to read from
    template<typename T = uint64_t>
    T decode(BitIStream& in) {
        T value = in.template read_delta<T>();
        --value;
        return value;
    }
};

}} // namespace tdc::code
