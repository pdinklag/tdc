#pragma once

#include <limits>

#include <tdc/mpf/mpfloat.hpp>
#include <tdc/mpf/random/uniform_distribution.hpp>
#include <tdc/mpf/random/normal_distribution.hpp>

namespace tdc {
namespace mpf {
namespace random {

// TODO: use urand from MPFR

template<typename T, mpfr_rnd_t m_rnd = MPFR_RNDN>
class BinomialDistribution {
private:
    // use a mantissa with a precision matching the amount of bits in T
    // however, use at least double precision (53-bit mantissa)
    static constexpr mpfr_prec_t m_prec = std::max(DOUBLE_PRECISION, (mpfr_prec_t)std::numeric_limits<T>::digits);

    using F = mpfloat_t<m_prec, m_rnd>;
    using urand_t = Uniform01Distribution<m_prec, m_rnd>;
    using nrand_t = Normal01Distribution<m_prec, m_rnd>;

    bool m_easy;
    
    T m_t;
    F m_p;
    
    F m_d1, m_d2, m_s1, m_s2, m_c, m_a1, m_a123, m_s, m_lf, m_lp1p, m_q;

    template<typename random_generator_t>
    T waiting(random_generator_t& rng, const T& t, const F& q) {
        T x(0);
        F sum(0);
        
        urand_t urand;
        do {
            if (t == x) {
                return x;
            }
            
            const F e = -F::log(1.0 - F(urand(rng)));
            sum += e / F(t - x);
            ++x;
        } while(sum <= q);
        
        return x - 1;
    }

public:
    BinomialDistribution(const T& t, const double p) : m_t(t), m_p(p) {
        const F p12 = m_p <= 0.5 ? m_p : 1.0 - m_p;
        const F _t = F(m_t);
        if(_t * p12 >= 8) {
            m_easy = false;
            
            const F np = F::floor(_t * p12);
            const F pa = np / _t;
            const F pa1 = 1.0 - pa;
            
            const F pi_4 = F::pi() / 4.0;
            const F d1x = F::sqrt(np * pa1 * F::log(32 * np / (81 * pi_4 * pa1)));
            m_d1 = F::round(F::max(1.0, d1x));
            
            const F d2x = F::sqrt(np * pa1 * F::log(32 * _t * pa1 / (pi_4 * pa)));
            m_d2 = F::round(F::max(1.0, d2x));
            
            const F spi_2 = F::sqrt(F::pi() / 2.0);
            m_s1 = F::sqrt(np * pa1) * (1 + m_d1 / (4 * np));
            m_s2 = F::sqrt(np * pa1) * (1 + m_d2 / (4 * _t * pa1));
            m_c = 2 * m_d1 / np;
            m_a1 = F::exp(m_c) * m_s1 * spi_2;
            const F a12 = m_a1 + m_s2 * spi_2;
            const F s1s = m_s1 * m_s1;
            m_a123 = a12 + (F::exp(m_d1 / (_t * pa1)) * 2 * s1s / m_d1 * F::exp(-m_d1 * m_d1 / (2 * s1s)));
            const F s2s = m_s2 * m_s2;
            m_s = (m_a123 + 2 * s2s / m_d2 * F::exp(-m_d2 * m_d2 / (2 * s2s)));
            m_lf = (F::lgamma(np + 1) + F::lgamma(_t - np + 1));
            m_lp1p = F::log(pa / pa1);

            m_q = -F::log(1 - (p12 - pa) / pa1);
        } else {
            m_easy = true;
            m_q = -F::log(1 - p12);
        }
    }
    
    template<typename random_generator_t>
    T operator()(random_generator_t& rng) {
        T ret;
        
        const T t = m_t;
        const F ft = F(t);
        const F p = m_p;
        const F p12 = p <= 0.5 ? p : 1.0 - p;
        
        urand_t urand;
        nrand_t nrand;
        
        if(!m_easy) {
            F x;

            const F naf(0.5); // TODO: (1-epsilon)/2
            const F thr = F(std::numeric_limits<T>::max()) + naf;

            const F np = F::floor(ft * p12);

            const F spi_2 = F::sqrt(F::pi() / 2.0);
            const F a1 = m_a1;
            const F a12 = a1 + m_s2 * spi_2;
            const F a123 = m_a123;
            const F s1s = m_s1 * m_s1;
            const F s2s = m_s2 * m_s2;

            bool reject;
            do {
                const F u = m_s * urand(rng);

                F v;

                if (u <= a1)
                {
                    const F n = nrand(rng);
                    const F y = m_s1 * F::abs(n);
                    reject = y >= m_d1;
                    if (!reject)
                    {
                        const F e = -F::log(1.0 - urand(rng));
                        x = F::floor(y);
                        v = -e - n * n / 2 + m_c;
                    }
                }
                else if (u <= a12)
                {
                    const F n = nrand(rng); // wtf is _M_nd?
                    const F y = m_s2 * F::abs(n);
                    reject = y >= m_d2;
                    if (!reject)
                    {
                        const F e = -F::log(1.0 - urand(rng));
                        x = F::floor(-y);
                        v = -e - n * n / 2;
                    }
                }
                else if (u <= a123)
                {
                    const F e1 = -F::log(1.0 - urand(rng));
                    const F e2 = -F::log(1.0 - urand(rng));

                    const F y = m_d1 + 2 * s1s * e1 / m_d1;
                    x = F::floor(y);
                    v = (-e2 + m_d1 * (1 / (ft - np) -y / (2 * s1s)));
                    reject = false;
                }
                else
                {
                    const F e1 = -F::log(1.0 - urand(rng));
                    const F e2 = -F::log(1.0 - urand(rng));

                    const F y = m_d2 + 2 * s2s * e1 / m_d2;
                    x = F::floor(-y);
                    v = -e2 - m_d2 * y / (2 * s2s);
                    reject = false;
                }

                reject = reject || x < -np || x > ft - np;
                if (!reject)
                {
                    const double lfx = F::lgamma(np + x + 1) + F::lgamma(ft - (np + x) + 1);
                    reject = v > m_lf - lfx + x * m_lp1p;
                }

                reject |= x + np >= thr;
            } while (reject);

            x += np + naf;

            const T ix = (T)x;
            const T z = waiting(rng, t - ix, m_q);
            ret = ix + z;
        } else {
            ret = waiting(rng, t, m_q);
        }
        
        if(p12 != p) {
            return t - ret;
        } else {
            return ret;
        }
    }
};

}}} // namespace tdc::mpf::random
