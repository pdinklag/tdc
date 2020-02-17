#include <iostream>
#include <tdc/rapl/reader.hpp>

using namespace tdc::rapl;

#ifdef TDC_RAPL_AVAILABLE

uint32_t Reader::m_num_packages = powercap_rapl_get_num_packages();
std::array<powercap_rapl_pkg, NUM_RAPL_PACKAGES> Reader::m_pkg;

Reader::zone_support Reader::supported_zones(uint32_t package) {
    zone_support s;
    s.package = powercap_rapl_is_zone_supported(&m_pkg[package], POWERCAP_RAPL_ZONE_PACKAGE);
    s.core    = powercap_rapl_is_zone_supported(&m_pkg[package], POWERCAP_RAPL_ZONE_CORE);
    s.uncore  = powercap_rapl_is_zone_supported(&m_pkg[package], POWERCAP_RAPL_ZONE_UNCORE);
    s.dram    = powercap_rapl_is_zone_supported(&m_pkg[package], POWERCAP_RAPL_ZONE_DRAM);
    s.psys    = powercap_rapl_is_zone_supported(&m_pkg[package], POWERCAP_RAPL_ZONE_PSYS);
    return s;
}

Reader::zone_support Reader::supported_zones() {
    zone_support s = supported_zones(0);
    zone_support spkg;
    for(uint32_t i = 1; i < m_num_packages; i++) {
        spkg = supported_zones(i);
        s.package = s.package && spkg.package;
        s.core    = s.core    && spkg.core;
        s.uncore  = s.uncore  && spkg.uncore;
        s.dram    = s.dram    && spkg.dram;
        s.psys    = s.psys    && spkg.psys;
    }
    return s;
}

energy Reader::read(uint32_t package) {
    energy e;
    powercap_rapl_get_energy_uj(&m_pkg[package], POWERCAP_RAPL_ZONE_PACKAGE, &e.package);
    powercap_rapl_get_energy_uj(&m_pkg[package], POWERCAP_RAPL_ZONE_CORE,    &e.core);
    powercap_rapl_get_energy_uj(&m_pkg[package], POWERCAP_RAPL_ZONE_UNCORE,  &e.uncore);
    powercap_rapl_get_energy_uj(&m_pkg[package], POWERCAP_RAPL_ZONE_DRAM,    &e.dram);
    powercap_rapl_get_energy_uj(&m_pkg[package], POWERCAP_RAPL_ZONE_PSYS,    &e.psys);
    return e;
}

energy_buffer Reader::read() {
    energy_buffer buf;
    for(uint32_t i = 0; i < m_num_packages; i++) {
        buf[i] = read(i);
    }
    return buf;
}

Reader::Reader() {
    if(NUM_RAPL_PACKAGES < m_num_packages) {
        std::cerr
            << "WARNING: There are more RAPL packages available (" << m_num_packages
            << ") than configured by the NUM_RAPL_PACKAGES macro (" << NUM_RAPL_PACKAGES
            << "). RAPL read results may be incomplete!"
            << std::endl;

        m_num_packages = NUM_RAPL_PACKAGES;
    }

    for(uint32_t i = 0; i < m_num_packages; i++) {
        powercap_rapl_init(i, &m_pkg[i], true);
    }
}

Reader::~Reader() {
    for(uint32_t i = 0; i < m_num_packages; i++) {
        powercap_rapl_destroy(&m_pkg[i]);
    }
}

#else

// RAPL not available - provide stubs

uint32_t Reader::m_num_packages = 0;

Reader::zone_support Reader::supported_zones(uint32_t package) {
    return zone_support{ false, false, false, false, false };
}

Reader::zone_support Reader::supported_zones() {
    return zone_support{ false, false, false, false, false };
}

energy Reader::read(uint32_t package) {
    return energy();
}

energy Reader::read() {
    return energy();
}

#endif

// instantiate singleton
Reader Reader::m_singleton;

const uint32_t& Reader::num_packages = Reader::m_num_packages;
