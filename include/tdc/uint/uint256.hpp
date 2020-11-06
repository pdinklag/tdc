#pragma once

#include <tdc/intrisics/lzcnt.hpp>
#include <tdc/intrisics/popcnt.hpp>
#include <tdc/intrisics/tzcnt.hpp>

#include <tdc/uint/uint128.hpp>
#include <tdc/uint/print_uint.hpp>

#include <cmath>
#include <stdexcept>
#include <utility>

namespace tdc {

// TODO: use AVX512 registers if available

/// \brief 256-bit unsigned integer type.
class uint256_t {
private:
    uint128_t m_lo, m_hi;

    static std::pair<uint256_t, uint256_t> divmod(const uint256_t& a, const uint256_t& b);
        
public:
    /// \brief Default initialization to zero.
    inline constexpr uint256_t() : uint256_t(0, 0) {
    }
    
    /// \brief Constructs a 256-bit value.
    /// \param lo the 128 low bits.
    /// \param hi the 128 high bits.
    inline constexpr uint256_t(uint128_t lo, uint128_t hi) : m_lo(lo), m_hi(hi) {
    }
    
    /// \brief Constructs a 256-bit value.
    /// \param lo the 128 low bits.
    /// \param hi the 128 high bits.
    inline constexpr uint256_t(uint64_t x0, uint64_t x1, uint64_t x2, uint64_t x3)
        : m_lo(uint128_t(x1) << 64ULL | uint128_t(x0)),
          m_hi(uint128_t(x3) << 64ULL | uint128_t(x2)) {
    }
    
    /// \brief Construct from a 32-bit signed integer.
    /// \param v the integer to construct from
    inline constexpr uint256_t(const int v) : uint256_t(v, v >= 0 ? 0 : UINT128_MAX) {
    }

    /// \brief Construct from a 32-bit unsigned integer.
    /// \param v the integer to construct from
    inline constexpr uint256_t(const unsigned int v) : uint256_t(v, 0) {
    }

    /// \brief Construct from a ??-bit unsigned integer
    /// \param v the integer to construct from
    inline constexpr uint256_t(const unsigned long int v) : uint256_t(v, 0) {
    }
    
    /// \brief Construct from an 64-bit unsigned integer
    /// \param v the integer to construct from
    inline constexpr uint256_t(const unsigned long long int v) : uint256_t(v, 0) {
    }
    
    /// \brief Construct from a 128-bit unsigned integer
    /// \param v the integer to construct from
    inline constexpr uint256_t(const uint128_t v) : uint256_t(v, 0) {
    }
    
    uint256_t(const uint256_t&) = default;
    uint256_t(uint256_t&&) = default;

    uint256_t& operator=(const uint256_t&) = default;
    uint256_t& operator=(uint256_t&&) = default;
    
    /// \brief Cast to a 128-bit integer.
    explicit inline constexpr operator uint128_t() const { return m_lo; }
    
    /// \brief Cast to a 64-bit integer.
    explicit inline constexpr operator uint64_t() const { return m_lo; }
    
    /// \brief Cast to a 32-bit integer.
    explicit inline constexpr operator uint32_t() const { return m_lo; }
    
    /// \brief Cast to a 16-bit integer.
    explicit inline constexpr operator uint16_t() const { return m_lo; }
    
    /// \brief Cast to a 8-bit integer.
    explicit inline constexpr operator uint8_t() const { return m_lo; }

    /// \brief Cast to boolean.
    explicit inline constexpr operator bool() const { return m_lo > 0 || m_hi > 0; }
    
    /// \brief Unary plus.
    inline constexpr uint256_t operator+() const {
        return uint256_t(m_lo, m_hi);
    }

    /// \brief Unary minus.
    inline constexpr uint256_t operator-() const {
        return ~*this + 1;
    }
    
    /// \brief Prefix increment.
    inline uint256_t& operator++() {
        ++m_lo;
        if(!m_lo) ++m_hi;
        return *this;
    }

    /// \brief Postfix increment.
    inline uint256_t operator++(int) {
        uint256_t before(m_lo, m_hi);
        ++*this;
        return before;
    }

    /// \brief Prefix decrement.
    inline uint256_t& operator--() {
        --m_lo;
        if(m_lo == UINT32_MAX) --m_hi;
        return *this;
    }

    /// \brief Postfix decrement.
    inline uint256_t operator--(int) {
        uint256_t before(m_lo, m_hi);
        --*this;
        return before;
    }
    
    /// \brief Addition.
    inline constexpr uint256_t operator+(const uint256_t& other) const {
        const uint128_t lo = m_lo + other.m_lo;
        const bool overflow = lo < (m_lo | other.m_lo);
        const uint128_t hi = m_hi + other.m_hi + overflow;
        return uint256_t(lo, hi);
    }
    
    /// \brief Addition assignment.
    inline uint256_t& operator+=(const uint256_t& other) {
        *this = *this + other;
        return *this;
    }
    
    /// \brief Subtraction.
    inline constexpr uint256_t operator-(const uint256_t& other) const {
        return *this + (-other);
    }
    
    /// \brief Subtraction assignment.
    inline uint256_t& operator-=(const uint256_t& other) {
        *this = *this - other;
        return *this;
    }

    /// \brief Multiplication.
    uint256_t operator*(const uint256_t& other) const;

    /// \brief Multiplication assignment.
    inline uint256_t& operator*=(const uint256_t& other) {
        *this = *this * other;
        return *this;
    }

    /// \brief Division.
    uint256_t operator/(const uint256_t& other) const {
        auto qr = divmod(*this, other);
        return qr.first;
    }

    /// \brief Division assignment.
    inline uint256_t& operator/=(const uint256_t& other) {
        *this = *this / other;
        return *this;
    }

    /// \brief Modulo.
    uint256_t operator%(const uint256_t& other) const {
        auto qr = divmod(*this, other);
        return qr.second;
    }

    /// \brief Modulo assignment.
    inline uint256_t& operator%=(const uint256_t& other) {
        *this = *this % other;
        return *this;
    }
    
    /// \brief Bitwise OR.
    inline constexpr uint256_t operator|(const uint256_t& other) const {
        return uint256_t(m_lo | other.m_lo, m_hi | other.m_hi);
    }

    /// \brief Bitwise OR assignment.
    inline uint256_t& operator|=(const uint256_t& other) {
        *this = *this | other;
        return *this;
    }

    /// \brief Bit negation.
    inline constexpr uint256_t operator~() const {
        return uint256_t(~m_lo, ~m_hi);
    }

    /// \brief Bitwise AND.
    inline constexpr uint256_t operator&(const uint256_t& other) const {
        return uint256_t(m_lo & other.m_lo, m_hi & other.m_hi);
    }

    /// \brief Bitwise AND assignment.
    inline uint256_t& operator&=(const uint256_t& other) {
        *this = *this & other;
        return *this;
    }

    /// \brief Bitwise XOR.
    inline constexpr uint256_t operator^(const uint256_t& other) const {
        return uint256_t(m_lo ^ other.m_lo, m_hi ^ other.m_hi);
    }

    /// \brief Bitwise XOR assignment.
    inline uint256_t& operator^=(const uint256_t& other) {
        *this = *this ^ other;
        return *this;
    }

    /// \brief Left shift.
    inline constexpr uint256_t operator<<(const uint64_t& lsh) const {
        if(lsh == 0) {
            return *this;
        } else if(lsh < 128) {
            const uint128_t lo_mask = ~(UINT128_MAX >> lsh);
            const uint64_t lo_rsh = 128ULL - lsh;
            return uint256_t(m_lo << lsh, m_hi << lsh | ((m_lo & lo_mask) >> lo_rsh));
        } else {
            return uint256_t(0, m_lo << (lsh - 128ULL));
        }
        
    }

    /// \brief Left shift assignment.
    inline uint256_t& operator<<=(const uint64_t& lsh) {
        *this = *this << lsh;
        return *this;
    }

    /// \brief Right shift.
    inline constexpr uint256_t operator>>(const uint64_t& rsh) const {
        if(rsh == 0) {
            return *this;
        } else if(rsh < 128) {
            const uint128_t hi_mask = UINT128_MAX >> (128ULL - rsh);
            const uint64_t hi_lsh = 128ULL - rsh;
            return uint256_t(m_lo >> rsh | ((m_hi & hi_mask) << hi_lsh), m_hi >> rsh);
        } else {
            return uint256_t(m_hi >> (rsh - 128ULL), 0);
        }
    }
    
    /// \brief Right shift assignment.
    inline uint256_t& operator>>=(const uint64_t& rsh) {
        *this = *this >> rsh;
        return *this;
    }
    
    inline constexpr uint256_t operator<<(const uint256_t& lsh) const { return *this << (uint64_t)lsh; } // if the shift operand doesn't fit into 64 bits, then it doesn't really matter anyway...
    inline uint256_t& operator<<=(const uint256_t& lsh) const { return (*this <<= (uint64_t)lsh); }
    inline constexpr uint256_t operator>>(const uint256_t& rsh) const { return *this >> (uint64_t)rsh; }
    inline uint256_t& operator>>=(const uint256_t& rsh) const { return (*this >>= (uint64_t)rsh); }
    
    /// \brief Equality check.
    inline constexpr bool operator==(const uint256_t& other) const {
        return (m_lo == other.m_lo) && (m_hi == other.m_hi);
    }

    /// \brief Non-equality check.
    inline constexpr bool operator!=(const uint256_t& other) const {
        return (m_lo != other.m_lo) || (m_hi != other.m_hi);
    }

    /// \brief Less-than check.
    inline constexpr bool operator<(const uint256_t& other) const {
        return (m_hi < other.m_hi) || (m_hi == other.m_hi && m_lo < other.m_lo);
    }

    /// \brief Less-than-or-equal check.
    inline constexpr bool operator<=(const uint256_t& other) const {
        return (m_hi < other.m_hi) || (m_hi == other.m_hi && m_lo <= other.m_lo);
    }

    /// \brief Greater-than check.
    inline constexpr bool operator>(const uint256_t& other) const {
        return (m_hi > other.m_hi) || (m_hi == other.m_hi && m_lo > other.m_lo);
    }

    /// \brief Greater-than-or-equal check.
    inline constexpr bool operator>=(const uint256_t& other) const {
        return (m_hi > other.m_hi) || (m_hi == other.m_hi && m_lo >= other.m_lo);
    }
} __attribute__((__packed__));

/// \cond INTERNAL
namespace intrisics {

template<>
constexpr size_t lzcnt(const uint256_t x) {
    const size_t hi = lzcnt((uint128_t)(x >> 128));
    return hi == 128ULL ? 128ULL + lzcnt((uint128_t)x) : hi;
};

template<>
constexpr size_t popcnt(const uint256_t x) {
    return popcnt((uint128_t)x) + popcnt((uint128_t)(x >> 128));
};

template<>
constexpr size_t tzcnt(const uint256_t x) {
    const size_t lo = tzcnt((uint128_t)x);
    return lo == 128ULL ? 128ULL + tzcnt((uint128_t)(x >> 128)) : lo;
};

} //namespace intrisics
/// \endcond INTERNAL

} // namespace tdc

#include <climits>
#include <ostream>

namespace std {

/// \brief Numeric limits support for \ref tdc::uint256_t.
template<>
class numeric_limits<tdc::uint256_t> {
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
    static constexpr int digits = CHAR_BIT * sizeof(tdc::uint256_t);
    static constexpr int digits10 = digits * log10(2);
    static constexpr int max_digits10 = 0;
    static constexpr int radix = 2;
    static constexpr int min_exponent = 0;
    static constexpr int min_exponent10 = 0;
    static constexpr int max_exponent = 0;
    static constexpr int max_exponent10 = 0;
    static constexpr bool traps = numeric_limits<uint64_t>::traps; // act like \c uint64_t    
    static constexpr bool tinyness_before = false;
    static constexpr tdc::uint256_t min() noexcept { return tdc::uint256_t(0); }
    static constexpr tdc::uint256_t lowest() noexcept { return tdc::uint256_t(0); }
    static constexpr tdc::uint256_t max() noexcept { return tdc::uint256_t(UINT128_MAX, UINT128_MAX); }
    static constexpr tdc::uint256_t epsilon() noexcept { return tdc::uint256_t(0); }
    static constexpr tdc::uint256_t round_error() noexcept { return tdc::uint256_t(0); }
    static constexpr tdc::uint256_t infinity() noexcept { return tdc::uint256_t(0); }
    static constexpr tdc::uint256_t quiet_NaN() noexcept { return tdc::uint256_t(0); }
    static constexpr tdc::uint256_t signaling_NaN() noexcept { return tdc::uint256_t(0); }
    static constexpr tdc::uint256_t denorm_min() noexcept { return tdc::uint256_t(0); }
};

/// \brief Standard output support  for \ref tdc::uint256_t.
inline ostream& operator<<(ostream& out, tdc::uint256_t v) {
    return tdc::print_uint(out, v);
}

} // namespace std

// sanity checks
static_assert(sizeof(tdc::uint256_t) == 32);
static_assert(std::numeric_limits<tdc::uint256_t>::digits == 256);

/// \brief Globally define 256-bit integers.
using uint256_t = tdc::uint256_t;

/// \brief The maximum value for 256-bit integers.
#define UINT256_MAX std::numeric_limits<uint256_t>::max()

