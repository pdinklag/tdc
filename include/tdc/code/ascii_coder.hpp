#pragma once

#include "binary_coder.hpp"

namespace tdc {
namespace code {

/// \brief Binary code using 8 bits per integer.
using ASCIICoder = BinaryCoder<8>;

}} // namespace tdc::code
