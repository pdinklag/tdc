#pragma once

#include <bit>
#include <concepts>
#include <limits>
#include <stdexcept>

#include <tlx/container/ring_buffer.hpp>

#include <tdc/uint/uint128.hpp>

namespace tdc {
namespace hash {

template<std::unsigned_integral char_t>
class RollingKarpRabinFingerprint {
private:
    static constexpr uint128_t m_base = uint128_t(1) << std::numeric_limits<char_t>::digits;
    static constexpr uint128_t m_prime = 9223372036854775837ULL; // 2^63 + 29

    uint128_t m_f0;
    tlx::RingBuffer<char_t> m_window;
    uint128_t m_fp;

public:
    RollingKarpRabinFingerprint() : m_window(0), m_fp(0), m_f0(0) {
    }

    RollingKarpRabinFingerprint(size_t window, uint64_t offset = 0) : m_window(window), m_fp(offset) {
        // compute the factor pow(m_base, m_window) used to phase out the initial character of the window
        // TODO: there is probably a better algorithm for this?
        m_f0 = m_base;
        for(unsigned int i = 0; i < window - 1; i++) {
            m_f0 *= m_base;
        }
    }
    
    RollingKarpRabinFingerprint(const RollingKarpRabinFingerprint&) = default;
    RollingKarpRabinFingerprint(RollingKarpRabinFingerprint&&) = default;
    RollingKarpRabinFingerprint& operator=(const RollingKarpRabinFingerprint&) = default;
    RollingKarpRabinFingerprint& operator=(RollingKarpRabinFingerprint&&) = default;
    
    char_t advance(const char_t c) {
        const char_t first = m_window.front();
        
        // advance fingerprint with new character
        m_fp = ((m_fp * m_base) + c) % m_prime;
        
        // phase out first character if buffer is full
        if(full()) {
            const uint128_t first_influence = (first * m_f0) % m_prime;
            if(first_influence < m_fp) {
                m_fp -= first_influence;
            } else {
                m_fp = m_prime - (first_influence - m_fp);
            }
            m_window.pop_front();
        }
        
        // append new character to buffer
        m_window.push_back(c);
        
        // return first character
        return first;
    }
    
    bool full() const { return m_window.size() == m_window.max_size(); }
    
    const tlx::RingBuffer<char_t>& window() const { return m_window; }
    uint64_t fingerprint() const { return (uint64_t)m_fp; }
    
    std::string str() const {
        std::string s;
        s.reserve(m_window.size());
        for(size_t i = 0; i < m_window.size(); i++) s.push_back((char)m_window[i]);
        return s;
    }
};

}} // namespace tdc::hash
