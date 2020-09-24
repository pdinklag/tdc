#pragma once

#include <cstdint>

namespace tdc {
namespace intrisics {

#ifdef __BMI2__
/// \brief Performs a parallel bit extraction.
///
/// The instructions gathers the bits defined by the mask from the given word into the least-significant bits of the result.
///
/// \param x the word to extract bits from
/// \param mask the extraction mask
uint64_t pext(const uint64_t x, const uint64_t mask);

#else
#pragma message "tdc::intrisics::pext not avaiable because BMI2 is not supported -- when building in Debug mode, you may have to pass -mbmi2 to the compiler"
#endif
    
}} // namespace tdc::intrisics
