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
    static uint32_t m_num_packages;

#ifdef TDC_RAPL_AVAILABLE
    static std::array<powercap_rapl_pkg, NUM_RAPL_PACKAGES> m_pkg;
#endif

    static Reader m_singleton;

    Reader();
    ~Reader();

public:
    /// \brief The number of RAPL packages available on this system, as reported by RAPL itself.
    static const uint32_t& num_packages;

    /// \brief Returns the zones supported by the specified package.
    /// \param package the package in question
    static zone_support supported_zones(uint32_t package);

    /// \brief Returns the zones supported by all packages.
    static zone_support supported_zones();

    /// \brief Reads the current energy counter for the specified package.
    /// \param package the package in question
    static energy read(uint32_t package);

    /// \brief Reads the current energy counter for all packages.
    static energy_buffer read();
};

}} // namespace tdc::rapl
