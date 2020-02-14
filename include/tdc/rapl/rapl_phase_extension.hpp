#pragma once

#include <tdc/stat/phase_extension.hpp>
#include "reader.hpp"

namespace tdc {
namespace rapl {

class PhaseExtension : public stat::PhaseExtension {
private:
    energy_buffer m_begin;

public:
    PhaseExtension();

    virtual void write(stat::json& data) override;
};

}} // namespace tdc::rapl
