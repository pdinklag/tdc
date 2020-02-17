#pragma once

#ifdef POWERCAP_FOUND
#ifndef NUM_RAPL_PACKAGES
#pragma message("The NUM_RAPL_PACKAGES macro isn't defined and will therefore default to 1.")
/// \brief Determined by the build system.
#define NUM_RAPL_PACKAGES 1
#endif
#else
/// \brief Determined by the build system.
#define NUM_RAPL_PACKAGES 0
#endif

namespace tdc {
namespace rapl {

/// \brief The number of available RAPL packages on this system, determined by the build system.
constexpr uint32_t num_packages = NUM_RAPL_PACKAGES;

}} // namespace tdc::rapl
