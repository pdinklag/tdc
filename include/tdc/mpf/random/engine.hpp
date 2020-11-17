#pragma once

#include <gmp.h>

namespace tdc {
namespace mpf {
namespace random {

/// \brief The default seed used for random generators.
constexpr unsigned long DEFAULT_SEED = 147ULL;

/// \brief Random number engine for mpfloats using GMP's default engine, which typically uses the Mersenne Twister algorithm.
///
/// This class is the mpfloat equivalent of the random engines in the STL, and is only compatible to mpfloat-based random distributions.
class Engine {
private:
    gmp_randstate_t m_state;

public:
    /// \brief Constructs a random engine.
    inline Engine(unsigned long seed = DEFAULT_SEED) {
        gmp_randinit_default(m_state);
        gmp_randseed_ui(m_state, seed);
    }

    inline ~Engine() {
        gmp_randclear(m_state);
    }

    /// \brief Provides access to the random state of the engine.
    inline gmp_randstate_t& state() {
        return m_state;
    }
};

}}}
