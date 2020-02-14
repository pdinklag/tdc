#include <tdc/rapl/rapl_phase_extension.hpp>

using namespace tdc::rapl;

PhaseExtension::PhaseExtension() : m_begin(Reader::read()) {
}

void PhaseExtension::write(stat::json& data) {
    auto stats = (Reader::read() - m_begin).total();

    data["energy_package"] = stats.package;
    data["energy_core"]    = stats.core;
    data["energy_uncore"]  = stats.uncore;
    data["energy_dram"]    = stats.dram;
    data["energy_psys"]    = stats.psys;
}
