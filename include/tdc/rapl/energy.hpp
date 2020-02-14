#pragma once

#include <cstdint>

namespace tdc {
namespace rapl {

/// \brief Contains an energy measurement for a single RAPL package.
struct energy {
    /// \brief The energy measurement for the entire package in microjoules.
    uint64_t package;

    /// \brief The energy measurement for the core zone in microjoules.
    uint64_t core;

    /// \brief The energy measurement for the uncore zone in microjoules.
    uint64_t uncore;

    /// \brief The energy measurement for the DRAM zone in microjoules.
    uint64_t dram;

    /// \brief The energy measurement for the PSYS zone in microjoules.
    uint64_t psys;

    inline energy() : package(0), core(0), uncore(0), dram(0), psys(0) {
    }

    inline energy(uint64_t _package, uint64_t _core, uint64_t _uncore, uint64_t _dram, uint64_t _psys)
        : package(_package), core(_core), uncore(_uncore), dram(_dram), psys(_psys) {
    }

    inline energy operator+(const energy& other) {
        return energy (
            package + other.package,
            core    + other.core,
            uncore  + other.uncore,
            dram    + other.dram,
            psys    + other.psys
        );
    }

    inline energy& operator+=(const energy& other) {
        package += other.package;
        core    += other.core;
        uncore  += other.uncore;
        dram    += other.dram;
        psys    += other.psys;
        return *this;
    }

    inline energy operator-(const energy& other) {
        return energy (
            package - other.package,
            core    - other.core,
            uncore  - other.uncore,
            dram    - other.dram,
            psys    - other.psys
        );
    }

    inline energy& operator-=(const energy& other) {
        package -= other.package;
        core    -= other.core;
        uncore  -= other.uncore;
        dram    -= other.dram;
        psys    -= other.psys;
        return *this;
    }
};

}} // namespace tdc::rapl

#include <ostream>

std::ostream& operator<<(std::ostream& os, const tdc::rapl::energy& e) {
    os << "(" << e.package << "," << e.core << "," << e.uncore << "," << e.dram << "," << e.psys << ")";
    return os;
}
