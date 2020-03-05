#include <tdc/rapl/rapl_phase_extension.hpp>

using namespace tdc::rapl;

const std::string RAPLPhaseExtension::STAT_ENERGY_PKG = "energyPackage";
const std::string RAPLPhaseExtension::STAT_ENERGY_CORE = "energyCore";
const std::string RAPLPhaseExtension::STAT_ENERGY_UNCORE = "energyUncore";
const std::string RAPLPhaseExtension::STAT_ENERGY_DRAM = "energyDRAM";
const std::string RAPLPhaseExtension::STAT_ENERGY_PSYS = "energyPSYS";

RAPLPhaseExtension::RAPLPhaseExtension() : m_begin(Reader::read()) {
}

void RAPLPhaseExtension::write(stat::json& data) {
    auto stats = (Reader::read() - m_begin).total();
    
    data[STAT_ENERGY_PKG]    = stats.package;
    data[STAT_ENERGY_CORE]   = stats.core;
    data[STAT_ENERGY_UNCORE] = stats.uncore;
    data[STAT_ENERGY_DRAM]   = stats.dram;
    data[STAT_ENERGY_PSYS]   = stats.psys;
}
