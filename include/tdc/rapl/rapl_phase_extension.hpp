#pragma once

#include <tdc/stat/phase_extension.hpp>
#include "reader.hpp"

namespace tdc {
namespace rapl {

/// \brief Stat phase extension for measuring energy using the RAPL interface.
///
/// Use \ref stat::Phase::register_extension to activate.
class RAPLPhaseExtension : public stat::PhaseExtension {
public:
    /// \brief The stat key used for a phase's package energy use.
    static const std::string STAT_ENERGY_PKG;
    /// \brief The stat key used for a phase's core energy use.
    static const std::string STAT_ENERGY_CORE;
    /// \brief The stat key used for a phase's uncore energy use.
    static const std::string STAT_ENERGY_UNCORE;
    /// \brief The stat key used for a phase's dram energy use.
    static const std::string STAT_ENERGY_DRAM;
    /// \brief The stat key used for a phase's psys energy use.
    static const std::string STAT_ENERGY_PSYS;

private:
    energy_buffer m_begin;

public:
    RAPLPhaseExtension();

    virtual void write(stat::json& data) override;
};

}} // namespace tdc::rapl
