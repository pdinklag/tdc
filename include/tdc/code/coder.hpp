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
    /// \brief Default constructor.
    inline Coder() {
    }

    Coder(const Coder& other) = default;
    Coder(Coder&& other) = default;
    Coder& operator=(const Coder& other) = default;
    Coder& operator=(Coder&& other) = default;

    /// \brief Initializes the coder for encoding.
    ///
    /// This function is used to "announce" to the coder that the given array is about to be encoded.
    /// It allows for initializations such as scanning the input alphabet and writing a header to the output stream.
    ///
    /// \tparam array_t the array type
    /// \param out the bit output stream into which to write a header
    /// \param array the array to be encoded
    /// \param num the number of items in the array
    template<typename array_t>
    inline void encode_init(BitOStream& out, const array_t& array, const size_t num) {
    }

    /// \brief Initializes the coder for decoding.
    ///
    /// The initialization may involve reading a header from the given input stream.
    ///
    /// \param in the bit input stream to read a header from
    inline void decode_init(BitIStream& in) {
    }

    /// \brief Tests whether the decoding of the given input stream has finished.
    /// \param in the input stream in question
    inline bool decode_eof(const BitIStream& in) const {
        return in.eof();
    }
};

}} // namespace tdc::code
