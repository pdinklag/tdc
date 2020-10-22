#pragma once

#include <cstdint>
#include <tdc/util/uint40.hpp>

namespace tdc {
namespace intrisics {

#ifdef __BMI2__

/// \brief Performs a parallel bit extraction.
///
/// The instructions gathers the bits defined by the mask from the given word into the least-significant bits of the result.
///
/// \tparam T the word type
/// \param x the word to extract bits from
/// \param mask the extraction mask
template<typename T>
T pext(const T x, const T mask);

#else
#pragma message "tdc::intrisics::pext not avaiable because BMI2 is not supported -- when building in Debug mode, you may have to pass -mbmi2 to the compiler"
#endif
    
}} // namespace tdc::intrisics
