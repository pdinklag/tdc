#pragma once

#include <array>

#include "config.hpp"
#include "energy.hpp"

namespace tdc {
namespace rapl {
    /// \brief A buffer of energy measurements for all packages.
    struct energy_buffer {
        /// \brief The measurements for each package.
        std::array<energy, num_packages> packages;

        inline energy_buffer() {
        }

        /// \brief Gets the energy measurement for the specified package.
        /// \param i the package in question, must be less than \c tdc::rapl::num_packages
        inline energy& operator[](size_t i) {
            return packages[i];
        }
        
        /// \brief Gets the read-only energy measurement for the specified package.
        /// \param i the package in question, must be less than \c tdc::rapl::num_packages
        inline const energy& operator[](size_t i) const {
            return packages[i];
        }

        inline energy_buffer operator-(const energy_buffer& other) {
            energy_buffer b;
            for(size_t i = 0; i < num_packages; i++) {
                b[i] = packages[i] - other.packages[i];
            }
            return b;
        }

        /// \brief Accumulates the energy measurements of all packages.
        inline energy total() {
            energy e;
            for(size_t i = 0; i < num_packages; i++) {
                e += packages[i];
            }
            return e;
        }
    };

}} // namespace tdc::rapl
