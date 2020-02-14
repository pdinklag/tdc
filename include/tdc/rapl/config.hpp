#pragma once

#ifndef NUM_RAPL_PACKAGES
#pragma message("The NUM_RAPL_PACKAGES macro isn't defined and will therefore default to 1.")
/// \brief The number of available RAPL packages on this system, determined by the build system.
#define NUM_RAPL_PACKAGES 1
#endif
