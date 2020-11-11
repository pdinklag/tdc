#pragma once

#include <tdc/mpf/mpfloat.hpp>

namespace tdc {
namespace mpf {
namespace random {

template<mpfr_prec_t m_precision, mpfr_rnd_t m_rnd = MPFR_RNDN>
class Normal01Distribution {
private:
    using F = mpfloat_t<m_precision, m_rnd>;

public:
    Normal01Distribution() {
    }

    template<typename random_engine_t>
    void generate(random_engine_t& rng, F& f) {
        mpfr_nrandom(f.mpfr(), rng.state(), m_rnd);
    }

    template<typename random_engine_t>
    F operator()(random_engine_t& rng) {
        F f;
        generate(rng, f);
        return f;
    }
};

}}} // namespace tdc::mpf::random
