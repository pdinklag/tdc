#pragma once

#include <bit>
#include <concepts>
#include <limits>
#include <stdexcept>

#include <tdc/uint/uint128.hpp>

namespace tdc {
namespace hash {

template<std::unsigned_integral char_t>
class RollingKarpRabinFingerprint {
private:
    static constexpr uint128_t m_base = uint128_t(1) << std::numeric_limits<char_t>::digits;
    static constexpr uint128_t m_prime = 18446744073709551253ULL;

    uint128_t m_fp, m_f0;

public:
    RollingKarpRabinFingerprint() : m_fp(0), m_f0(0) {
    }

    RollingKarpRabinFingerprint(size_t window, uint64_t offset = 0) : m_fp(offset) {
        // compute the factor pow(m_base, m_window) used to phase out the initial character of the window
        // TODO: there is probably a better algorithm for this?
        m_f0 = uint128_t(1);
        for(size_t i = 0; i < window; i++) {
            m_f0 = (m_f0 * m_base) % m_prime;
        }
    }
    
    RollingKarpRabinFingerprint(const RollingKarpRabinFingerprint&) = default;
    RollingKarpRabinFingerprint(RollingKarpRabinFingerprint&&) = default;
    RollingKarpRabinFingerprint& operator=(const RollingKarpRabinFingerprint&) = default;
    RollingKarpRabinFingerprint& operator=(RollingKarpRabinFingerprint&&) = default;
    
    void advance(const char_t in, const char_t out) {
        // advance fingerprint with in character
        m_fp = (m_fp * m_base + uint128_t(in)) % m_prime;
        
        // calculate away out character
        if(out) {
            const uint128_t out_influence = (uint128_t(out) * m_f0) % m_prime;
            if(out_influence < m_fp) {
                m_fp -= out_influence;
            } else {
                m_fp = m_prime - (out_influence - m_fp);
            }
        }
    }
    
    uint64_t fingerprint() const { return (uint64_t)m_fp; }
};

}} // namespace tdc::hash
