#pragma once

#include <cstdint>
#include <iostream>
#include <tdc/util/likely.hpp>

namespace tdc {
    /// \brief 40-bit unsigned integer type.
    #if defined(_MSC_VER)
    #pragma pack(push, 1)
    #endif
    class uint40_t {
    private:
        static constexpr uint32_t lo_max = UINT32_MAX;
        static constexpr uint8_t hi_max = UINT8_MAX;
    
        uint32_t m_lo;
        uint8_t  m_hi;

    public:
        /// \brief Default initialization to zero.
        inline constexpr uint40_t() : uint40_t(0, 0) {
        }

        /// \brief Constructs a 40-bit value out of its lowest 32 and highest 8 bits.
        /// \param lo the 32 low bits.
        /// \param hi the 8 high bits.
        inline constexpr uint40_t(uint32_t lo, uint8_t hi) : m_lo(lo), m_hi(hi) {
        }

        /// \brief Construct from an 32-bit signed integer.
        /// \param v the integer to construct from
        inline constexpr uint40_t(const int v) : uint40_t(v, v >= 0 ? 0 : hi_max) {
        }

        /// \brief Construct from a 32-bit unsigned integer.
        /// \param v the integer to construct from
        inline constexpr uint40_t(const unsigned int v) : uint40_t(v, 0) {
        }

        /// \brief Construct from a ??-bit unsigned integer, truncating its 24 most significant bits
        /// \param v the integer to construct from
        inline constexpr uint40_t(const unsigned long int v) : uint40_t(v, v >> 32ULL) {
        }
        
        /// \brief Construct from an 64-bit unsigned integer, truncating its 24 most significant bits
        /// \param v the integer to construct from
        inline constexpr uint40_t(const unsigned long long int v) : uint40_t(v, v >> 32ULL) {
        }

        uint40_t(const uint40_t&) = default;
        uint40_t(uint40_t&&) = default;

        uint40_t& operator=(const uint40_t&) = default;
        uint40_t& operator=(uint40_t&&) = default;

        /// \brief Convert to a 64-bit integer.
        inline uint64_t u64() const {
            return ((uint64_t)m_hi << 32ULL) | (uint64_t)m_lo;
        }

        /// \brief Cast to a 64-bit integer.
        inline operator uint64_t() const {
            return u64();
        }

        /// \brief Cast to boolean.
        inline operator bool() const {
            return m_lo > 0 || m_hi > 0;
        }

        /// \brief Unary plus.
        inline uint40_t operator+() {
            return uint40_t(m_lo, m_hi);
        }

        /// \brief Unary minus.
        inline uint40_t operator-() {
            return uint40_t(0) - *this;
        }

        /// \brief Prefix increment.
        inline uint40_t& operator++() {
            ++m_lo;
            if(tdc_unlikely(!m_lo)) ++m_hi;
            return *this;
        }

        /// \brief Postfix increment.
        inline uint40_t operator++(int) {
            uint40_t before(m_lo, m_hi);
            ++*this;
            return before;
        }

        /// \brief Prefix decrement.
        inline uint40_t& operator--() {
            --m_lo;
            if(tdc_unlikely(m_lo == lo_max)) ++m_hi;
            return *this;
        }

        /// \brief Postfix decrement.
        inline uint40_t operator--(int) {
            uint40_t before(m_lo, m_hi);
            --*this;
            return before;
        }

        /// \brief Addition.
        inline uint40_t operator+(const uint40_t& other) const {
            uint64_t sum64 = (uint64_t)m_lo + (uint64_t)other.m_lo;
            return uint40_t(sum64, m_hi + other.m_hi + (uint8_t)(sum64 >> 32ULL));
        }

        /// \brief Addition assignment.
        inline uint40_t& operator+=(const uint40_t& other) {
            *this = *this + other;
            return *this;
        }

        /// \brief Subtraction.
        inline uint40_t operator-(const uint40_t& other) const {
            uint64_t diff64 = (uint64_t)m_lo - (uint64_t)other.m_lo;
            return uint40_t(diff64, m_hi - other.m_hi + (uint8_t)(diff64 >> 32ULL));
        }

        /// \brief Subtraction assignment.
        inline uint40_t& operator-=(const uint40_t& other) {
            *this = *this - other;
            return *this;
        }

        /// \brief Multiplication.
        inline uint40_t operator*(const uint40_t& other) const {
            return uint40_t(u64() * other.u64());
        }

        /// \brief Multiplication assignment.
        inline uint40_t& operator*=(const uint40_t& other) {
            *this = *this * other;
            return *this;
        }

        /// \brief Integer division.
        inline uint40_t operator/(const uint40_t& other) const {
            return uint40_t(u64() / other.u64());
        }

        /// \brief Integer division assignment.
        inline uint40_t& operator/=(const uint40_t& other) {
            *this = *this / other;
            return *this;
        }

        /// \brief Modulo.
        inline uint40_t operator%(const uint40_t& other) const {
            return uint40_t(u64() % other.u64());
        }

        /// \brief Integer division assignment.
        inline uint40_t& operator%=(const uint40_t& other) {
            *this = *this % other;
            return *this;
        }

        /// \brief Bitwise OR.
        inline uint40_t operator|(const uint40_t& other) const {
            return uint40_t(m_lo | other.m_lo, m_hi | other.m_hi);
        }

        /// \brief Bitwise OR assignment.
        inline uint40_t& operator|=(const uint40_t& other) {
            *this = *this | other;
            return *this;
        }

        /// \brief Bit negation.
        inline uint40_t operator~() const {
            return uint40_t(~m_lo, ~m_hi);
        }

        /// \brief Bitwise AND.
        inline uint40_t operator&(const uint40_t& other) const {
            return uint40_t(m_lo & other.m_lo, m_hi & other.m_hi);
        }

        /// \brief Bitwise AND assignment.
        inline uint40_t& operator&=(const uint40_t& other) {
            *this = *this & other;
            return *this;
        }

        /// \brief Bitwise XOR.
        inline uint40_t operator^(const uint40_t& other) const {
            return uint40_t(m_lo ^ other.m_lo, m_hi ^ other.m_hi);
        }

        /// \brief Bitwise AND assignment.
        inline uint40_t& operator^=(const uint40_t& other) {
            *this = *this ^ other;
            return *this;
        }

        /// \brief Left shift.
        inline uint40_t operator<<(const uint64_t& lsh) const {
            if(tdc_likely(lsh)) {
                const uint8_t  hi = (m_hi << lsh) | (uint8_t)(m_lo >> (32ULL - lsh));
                const uint32_t lo = m_lo << lsh;
                return uint40_t(lo, hi);
            } else {
                return uint40_t(m_lo, m_hi);
            }
        }

        /// \brief Left shift assignment.
        inline uint40_t& operator<<=(const uint64_t& lsh) {
            *this = *this << lsh;
            return *this;
        }

        /// \brief Right shift.
        inline uint40_t operator>>(const uint64_t& rsh) const {
            if(tdc_likely(rsh)) {
                const uint32_t lo = (m_lo >> rsh) | (((uint32_t)m_hi & rsh) << (32ULL - rsh));
                const uint8_t  hi = m_hi >> rsh;
                return uint40_t(lo, hi);
            } else {
                return uint40_t(m_lo, m_hi);
            }
        }
        
        /// \brief Right shift assignment.
        inline uint40_t& operator>>=(const uint64_t& rsh) {
            *this = *this >> rsh;
            return *this;
        }

        /// \brief Equality check.
        inline bool operator==(const uint40_t& other) const {
            return (m_lo == other.m_lo) && (m_hi == other.m_hi);
        }

        /// \brief Non-equality check.
        inline bool operator!=(const uint40_t& other) const {
            return (m_lo != other.m_lo) || (m_hi != other.m_hi);
        }

        /// \brief Less-than check.
        inline bool operator<(const uint40_t& other) const {
            return (m_hi < other.m_hi) || (m_hi == other.m_hi && m_lo < other.m_lo);
        }

        /// \brief Less-than-or-equal check.
        inline bool operator<=(const uint40_t& other) const {
            return (m_hi <= other.m_hi) || (m_hi == other.m_hi && m_lo <= other.m_lo);
        }

        /// \brief Greater-than check.
        inline bool operator>(const uint40_t& other) const {
            return (m_hi > other.m_hi) || (m_hi == other.m_hi && m_lo > other.m_lo);
        }

        /// \brief Greater-than-or-equal check.
        inline bool operator>=(const uint40_t& other) const {
            return (m_hi >= other.m_hi) || (m_hi == other.m_hi && m_lo >= other.m_lo);
        }

        /// \brief Support for \c std::ostream output.
        friend std::ostream& operator<<(std::ostream&, const uint40_t&);

        /// \brief Support for \c std::istream input.
        friend std::istream& operator>>(std::istream&, uint40_t&);
        
    } /** \cond INTERNAL */ __attribute__ ((packed)) /** \endcond */;
    #if defined(_MSC_VER)
    #pragma pack(pop)
    #endif
} // namespace tdc

#include <climits>
#include <cmath>
#include <limits>

namespace std {

using uint40_t = tdc::uint40_t;

/// \brief Numeric limits support for \ref tdc::uint40_t.
///
/// In general, we want to act like \c uint64_t with some additional constraints on the value range.
template<>
class numeric_limits<tdc::uint40_t> {
public:
    static constexpr bool is_specialized = true;
    static constexpr bool is_signed = false;
    static constexpr bool is_integer = true;
    static constexpr bool is_exact = true;
    static constexpr bool has_infinity = false;
    static constexpr bool has_quiet_NaN = false;
    static constexpr bool has_signaling_NaN = false;
    static constexpr float_denorm_style has_denorm = denorm_absent;
    static constexpr bool has_denorm_loss = false;
    static constexpr float_round_style round_style = round_toward_zero;
    static constexpr bool is_iec559 = false;
    static constexpr bool is_bounded = true;
    static constexpr bool is_modulo = true;
    static constexpr int digits = CHAR_BIT * sizeof(uint40_t);
    static constexpr int digits10 = digits * log10(2);
    static constexpr int max_digits10 = 0;
    static constexpr int radix = 2;
    static constexpr int min_exponent = 0;
    static constexpr int min_exponent10 = 0;
    static constexpr int max_exponent = 0;
    static constexpr int max_exponent10 = 0;
    static constexpr bool traps = numeric_limits<uint64_t>::traps; // act like \c uint64_t    
    static constexpr bool tinyness_before = false;
    static constexpr uint40_t min() noexcept { return uint40_t(0); }
    static constexpr uint40_t lowest() noexcept { return uint40_t(0); }
    static constexpr uint40_t max() noexcept { return uint40_t(UINT32_MAX, UINT8_MAX); }
    static constexpr uint40_t epsilon() noexcept { return uint40_t(0); }
    static constexpr uint40_t round_error() noexcept { return uint40_t(0); }
    static constexpr uint40_t infinity() noexcept { return uint40_t(0); }
    static constexpr uint40_t quiet_NaN() noexcept { return uint40_t(0); }
    static constexpr uint40_t signaling_NaN() noexcept { return uint40_t(0); }
    static constexpr uint40_t denorm_min() noexcept { return uint40_t(0); }
};

inline ostream& operator<<(ostream& out, const uint40_t& v) {
    out << v.u64();
    return out;
}

inline istream& operator>>(istream& in, uint40_t& v) {
    uint64_t u64;
    in >> u64;
    v = uint40_t(u64);
    return in;
}

} // namespace std

// sanity checks
static_assert(sizeof(tdc::uint40_t) == 5);
static_assert(std::numeric_limits<tdc::uint40_t>::digits == 40);
