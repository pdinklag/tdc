#pragma once

#include <tdc/mpf/mpfloat.hpp>

namespace tdc {
namespace mpf {
namespace random {

template<mpfr_prec_t m_precision, mpfr_rnd_t m_rnd = MPFR_RNDN>
class Uniform01Distribution {
private:
    using F = mpfloat_t<m_precision, m_rnd>;

public:
    Uniform01Distribution() {
    }

    template<typename random_engine_t>
    void generate(random_engine_t& rng, F& f) {
        mpfr_urandomb(f.mpfr(), rng.state());
    }

    template<typename random_engine_t>
    F operator()(random_engine_t& rng) {
        F f;
        generate(rng, f);
        return f;
    }
};

template<mpfr_prec_t m_precision, mpfr_rnd_t m_rnd = MPFR_RNDN>
class UniformDistribution {
private:
    using F = mpfloat_t<m_precision, m_rnd>;
    using urand_t = Uniform01Distribution<m_precision, m_rnd>;

    F m_a, m_b, m_delta;

    void init_delta() {
        m_delta = m_b;
        m_delta -= m_a;
    }

public:
    UniformDistribution(const double a, const double b) : m_a(a), m_b(b) {
        init_delta();
    }

    UniformDistribution(const F& a, const F& b) : m_a(a), m_b(b) {
        init_delta();
    }

    UniformDistribution(F&& a, F&& b) : m_a(std::move(a)), m_b(std::move(b)) {
        init_delta();
    }

    template<typename random_engine_t>
    void generate(random_engine_t& rng, F& f) {
        urand_t urand;
        urand.generate(rng, f);
        f *= m_delta;
        f += m_a;
    }

    template<typename random_engine_t>
    F operator()(random_engine_t& rng) {
        F f;
        generate(rng, f);
        return f;
    }
};

}}} // namespace tdc::mpf::random
