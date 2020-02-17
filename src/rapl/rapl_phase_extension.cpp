#include <tdc/rapl/rapl_phase_extension.hpp>

using namespace tdc::rapl;

RAPLPhaseExtension::RAPLPhaseExtension() : m_begin(Reader::read()) {
}

void RAPLPhaseExtension::write(stat::json& data) {
    auto stats = (Reader::read() - m_begin).total();
    
    data["energyPackage"] = stats.package;
    data["energyCore"]    = stats.core;
    data["energyUncore"]  = stats.uncore;
    data["energyDRAM"]    = stats.dram;
    data["energyPSYS"]    = stats.psys;
}
