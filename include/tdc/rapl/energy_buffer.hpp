#pragma once

#include <array>

#include "config.hpp"
#include "energy.hpp"

namespace tdc {
namespace rapl {
    /// \brief A buffer of energy measurements for all packages.
    struct energy_buffer {
        /// \brief The measurements for each package.
        std::array<energy, NUM_RAPL_PACKAGES> packages;

        inline energy_buffer() {
        }

        /// \brief Gets the energy measurement for the specified package.
        /// \param i the package in question
        inline energy& operator[](size_t i) {
            return packages[i];
        }
        
        /// \brief Gets the read-only energy measurement for the specified package.
        /// \param i the package in question
        inline const energy& operator[](size_t i) const {
            return packages[i];
        }

        inline energy_buffer operator-(const energy_buffer& other) {
            energy_buffer b;
            for(size_t i = 0; i < NUM_RAPL_PACKAGES; i++) {
                b[i] = packages[i] - other.packages[i];
            }
            return b;
        }

        /// \brief Accumulates the energy measurements of all packages.
        inline energy total() {
            energy e;
            for(size_t i = 0; i < NUM_RAPL_PACKAGES; i++) {
                e += packages[i];
            }
            return e;
        }
    };

}} // namespace tdc::rapl
