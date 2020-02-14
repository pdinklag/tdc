#pragma once

#include "json.hpp"

namespace tdc {
namespace stat {

/// \brief Virtual interface for statistics phase extensions.
///
/// An extension can be used to add additional measurement features when a \c Phase is started,
/// e.g., energy or I/O operations.
/// Extensions are activated by registering them via \c Phase::register_extension.
///
/// When constructed, measurement shall start.
/// During the extension's lifetime, multiple calls to \c PhaseExtension::write may occur.
class PhaseExtension {
public:
    /// \brief Writes phase data to a JSON object.
    /// \param data the JSON object to write to
    virtual void write(json& data) = 0;

    /// \brief Propagates the data of a sub phase to this phase.
    /// \param sub the corresponding extension of the sub phase
    virtual void propagate(const PhaseExtension& sub) {
    }

    /// \brief Notifies the extension that tracking is being paused explicitly.
    ///
    /// While paused, statistics events related to this extension should not be counted.
    virtual void pause() {
    }

    /// \brief Notifies the extension that paused tracking is being resumed.
    ///
    /// This happens after a pause and indicates that tracking should resume.
    virtual void resume() {
    }
};

}} // namespace tdc::stat
