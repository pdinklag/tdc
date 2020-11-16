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

template<mpfr_prec_t m_precision, mpfr_rnd_t m_rnd = MPFR_RNDN>
class NormalDistribution {
private:
    using F = mpfloat_t<m_precision, m_rnd>;
    
    F m_mean, m_stddev;
    
public:
    template<typename int_t>
    NormalDistribution(const int_t& mean, const int_t& stddev) : m_mean(mean), m_stddev(stddev) {
    }

    NormalDistribution(const double mean, const double stddev) : m_mean(mean), m_stddev(stddev) {
    }
    
    NormalDistribution(const F& mean, const F& stddev) : m_mean(mean), m_stddev(stddev) {
    }

    NormalDistribution(F&& mean, F&& stddev) : m_mean(std::move(mean)), m_stddev(std::move(stddev)) {
    }

    template<typename random_engine_t>
    void generate(random_engine_t& rng, F& f) {
        mpfr_nrandom(f.mpfr(), rng.state(), m_rnd);
        f *= m_stddev;
        f += m_mean;
    }

    template<typename random_engine_t>
    F operator()(random_engine_t& rng) {
        F f;
        generate(rng, f);
        return f;
    }
};

}}} // namespace tdc::mpf::random
