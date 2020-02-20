#pragma once

#include <tdc/stat/phase_extension.hpp>
#include "reader.hpp"

namespace tdc {
namespace rapl {

/// \brief Stat phase extension for measuring energy using the RAPL interface.
///
/// Use \ref stat::Phase::register_extension to activate.
class RAPLPhaseExtension : public stat::PhaseExtension {
private:
    energy_buffer m_begin;

public:
    RAPLPhaseExtension();

    virtual void write(stat::json& data) override;
};

}} // namespace tdc::rapl
