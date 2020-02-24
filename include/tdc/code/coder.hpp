#pragma once

#include <tdc/io/bit_istream.hpp>
#include <tdc/io/bit_ostream.hpp>

namespace tdc {
namespace code {

using io::BitIStream;
using io::BitOStream;

/// \brief Base for coders.
class Coder {
public:
    /// \brief Constructs an uninitialized coder.
    inline Coder() {
    }

    /// \brief Constructs a coder and initializes it for encoding.
    ///
    /// This constructor is used to "announce" to the coder that the given array is about to be encoded.
    /// It allows for initializations such as scanning the input alphabet and writing a header to the output stream.
    ///
    /// \tparam array_t the array type
    /// \param array the array to be encoded
    /// \param num the number of items in the array
    /// \param out the bit output stream into which to write a header
    template<typename array_t>
    inline Coder(const array_t& array, const size_t num, BitOStream& out) : Coder() {
    }

    /// \brief Constructs a coder and initializes it for decoding.
    ///
    /// The initialization may involve reading a header from the given input stream.
    ///
    /// \param in the bit input stream to ready a header from
    inline Coder(BitIStream& in) : Coder() {
    }

    /// \brief Tests whether the decoding of the given input stream has finished.
    /// \param in the input stream in question
    inline bool eof(const BitIStream& in) const {
        return in.eof();
    }
};

}} // namespace tdc::code
