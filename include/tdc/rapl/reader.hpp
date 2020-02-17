#pragma once

#include <array>
#include "config.hpp"
#include "energy_buffer.hpp"

#ifdef POWERCAP_FOUND
#define TDC_RAPL_AVAILABLE
#include <powercap/powercap-rapl.h>
#endif

namespace tdc {
namespace rapl {

/// \brief A reader for RAPL energy measurements.
class Reader {
public:
    /// \brief Contains information about what zones a package support.
    struct zone_support {
        /// \brief Whether the package supports the accumulated package zone.
        bool package : 1;
        
        /// \brief Whether the package supports the core zone.
        bool core    : 1;

        /// \brief Whether the package supports the uncore zone.
        bool uncore  : 1;

        /// \brief Whether the package supports the DRAM zone.
        bool dram    : 1;

        /// \brief Whether the package supports the PSYS zone.
        bool psys    : 1;
    };

private:
#ifdef TDC_RAPL_AVAILABLE
    static std::array<powercap_rapl_pkg, num_packages> m_pkg;
#endif

    static Reader m_singleton;

    Reader();
    ~Reader();

public:
    /// \brief Returns the zones supported by the specified package.
    /// \param package the package in question, must be less than \c tdc::rapl::num_packages
    static zone_support supported_zones(uint32_t package);

    /// \brief Returns the zones supported by all packages.
    static zone_support supported_zones();

    /// \brief Reads the current energy counter for the specified package.
    /// \param package the package in question, must be less than \c tdc::rapl::num_packages
    static energy read(uint32_t package);

    /// \brief Reads the current energy counter for all packages.
    static energy_buffer read();
};

}} // namespace tdc::rapl
