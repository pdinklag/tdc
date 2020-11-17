#pragma once

#include <tdc/uint/print_uint.hpp>

#include <tdc/intrisics/lzcnt.hpp>
#include <tdc/intrisics/popcnt.hpp>
#include <tdc/intrisics/tzcnt.hpp>

#include <tdc/math/ilog2.hpp>

#include <cmath>
#include <limits>
#include <ostream>
#include <type_traits>
#include <utility>

namespace tdc {

template<size_t m_exp>
class uint_pow2_t;

} // namespace tdc

namespace std {

/// \brief Numeric limits support for \ref tdc::uint_pow2_t.
template<size_t m_exp>
class numeric_limits<tdc::uint_pow2_t<m_exp>> {
private:
    using uint_t = tdc::uint_pow2_t<m_exp>;
    using half_t = typename uint_t::half_t;
    
    static constexpr half_t HALF_MAX = numeric_limits<half_t>::max();

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
    static constexpr int digits = m_exp;
    static constexpr int digits10 = digits * log10(2);
    static constexpr int max_digits10 = 0;
    static constexpr int radix = 2;
    static constexpr int min_exponent = 0;
    static constexpr int min_exponent10 = 0;
    static constexpr int max_exponent = 0;
    static constexpr int max_exponent10 = 0;
    static constexpr bool traps = numeric_limits<uint64_t>::traps; // act like \c uint64_t    
    static constexpr bool tinyness_before = false;
    static constexpr uint_t min() noexcept { return uint_t(0); }
    static constexpr uint_t lowest() noexcept { return uint_t(0); }
    static constexpr uint_t max() noexcept { return uint_t(HALF_MAX, HALF_MAX); }
    static constexpr uint_t epsilon() noexcept { return uint_t(0); }
    static constexpr uint_t round_error() noexcept { return uint_t(0); }
    static constexpr uint_t infinity() noexcept { return uint_t(0); }
    static constexpr uint_t quiet_NaN() noexcept { return uint_t(0); }
    static constexpr uint_t signaling_NaN() noexcept { return uint_t(0); }
    static constexpr uint_t denorm_min() noexcept { return uint_t(0); }
};

/// \brief STL definitions.
template<size_t m_exp>
struct is_arithmetic<tdc::uint_pow2_t<m_exp>> {
    static constexpr bool value = true;
    
    inline operator bool() { return value; }
    inline bool operator()() { return value; }
};

/// \brief STL definitions.
template<size_t m_exp>
struct is_integral<tdc::uint_pow2_t<m_exp>> {
    static constexpr bool value = true;
    
    inline operator bool() { return value; }
    inline bool operator()() { return value; }
};

/// \brief STL definitions.
template<size_t m_exp>
struct is_scalar<tdc::uint_pow2_t<m_exp>> {
    static constexpr bool value = true;
    
    inline operator bool() { return value; }
    inline bool operator()() { return value; }
};

} // namespace std

namespace tdc {

template<size_t m_exp>
struct uint_pow2_half {
    static_assert(m_exp >= 64 && __builtin_popcountll(m_exp) == 1);
    using type = typename std::conditional<m_exp == 64, uint32_t, typename std::conditional<m_exp == 128, uint64_t, uint_pow2_t<m_exp / 2>>::type>::type;
};

template<size_t m_exp>
class uint_pow2_t {
public:
    static_assert(m_exp >= 64 && __builtin_popcountll(m_exp) == 1);
    
    using half_t = typename uint_pow2_half<m_exp>::type;
    static constexpr half_t HALF_MAX = std::numeric_limits<half_t>::max();
    static constexpr size_t HALF_BITS = std::numeric_limits<half_t>::digits;

    using quater_t = typename uint_pow2_half<m_exp / 2>::type;
    static constexpr quater_t QUARTER_MAX = std::numeric_limits<quater_t>::max();
    static constexpr size_t QUARTER_BITS = std::numeric_limits<quater_t>::digits;

private:
    half_t m_lo, m_hi;

public:
    /// \brief Default initialization to zero.
    constexpr uint_pow2_t() : uint_pow2_t(0, 0) {
    }
    
    /// \brief Constructs a value from low and high half components.
    /// \param lo the low bits
    /// \param hi the high bits
    constexpr uint_pow2_t(half_t lo, half_t hi) : m_lo(lo), m_hi(hi) {
    }
    
    /// \brief Constructs a value from quarter components in low-endian order.
    constexpr uint_pow2_t(quater_t x0, quater_t x1, quater_t x2, quater_t x3)
        : m_lo(half_t(x1) << QUARTER_BITS | half_t(x0)),
          m_hi(half_t(x3) << QUARTER_BITS | half_t(x2)) {
    }
    
    /// \brief Construct from a 32-bit signed integer.
    /// \param v the integer to construct from
    constexpr uint_pow2_t(const int32_t v) : uint_pow2_t(v, v >= 0 ? 0 : HALF_MAX) {
    }

    /// \brief Construct from a 32-bit unsigned integer.
    /// \param v the integer to construct from
    constexpr uint_pow2_t(const uint32_t v) : uint_pow2_t(v, 0) {
    }
    
    /// \brief Construct from an 64-bit unsigned integer.
    /// \param v the integer to construct from
    constexpr uint_pow2_t(const uint64_t v) : uint_pow2_t(v, 0) {
    }
    
    /// \brief Construct from a half value.
    /// \param v the integer to construct from
    template<size_t exp = m_exp>
    explicit constexpr uint_pow2_t(const typename std::enable_if<exp >= 256, half_t >::type& v) : uint_pow2_t(v, 0) {
    }
    
    uint_pow2_t(const uint_pow2_t&) = default;
    uint_pow2_t(uint_pow2_t&&) = default;

    uint_pow2_t& operator=(const uint_pow2_t&) = default;
    uint_pow2_t& operator=(uint_pow2_t&&) = default;
    
    /// \brief Cast to a half value.
    template<size_t exp = m_exp>
    explicit constexpr operator typename std::enable_if<exp >= 256, half_t >::type() const { return m_lo; }
    
    /// \brief Cast to a 64-bit integer.
    explicit constexpr operator uint64_t() const { return (uint64_t)m_lo; }
    
    /// \brief Cast to a 32-bit integer.
    explicit constexpr operator uint32_t() const { return (uint32_t)m_lo; }
    
    /// \brief Cast to a 16-bit integer.
    explicit constexpr operator uint16_t() const { return (uint16_t)m_lo; }
    
    /// \brief Cast to a 8-bit integer.
    explicit constexpr operator uint8_t() const { return (uint8_t)m_lo; }

    /// \brief Cast to boolean.
    explicit constexpr operator bool() const { return m_lo > 0 || m_hi > 0; }
    
    /// \brief Unary plus.
    constexpr uint_pow2_t operator+() const {
        return uint_pow2_t(m_lo, m_hi);
    }

    /// \brief Unary minus.
    constexpr uint_pow2_t operator-() const {
        return ~*this + 1;
    }
    
    /// \brief Prefix increment.
    uint_pow2_t& operator++() {
        ++m_lo;
        if(!m_lo) ++m_hi;
        return *this;
    }

    /// \brief Postfix increment.
    uint_pow2_t operator++(int) {
        uint_pow2_t before(m_lo, m_hi);
        ++*this;
        return before;
    }

    /// \brief Prefix decrement.
    uint_pow2_t& operator--() {
        --m_lo;
        if(m_lo == HALF_MAX) --m_hi;
        return *this;
    }

    /// \brief Postfix decrement.
    uint_pow2_t operator--(int) {
        uint_pow2_t before(m_lo, m_hi);
        --*this;
        return before;
    }
    
    /// \brief Addition.
    constexpr uint_pow2_t operator+(const uint_pow2_t& other) const {
        const half_t lo = m_lo + other.m_lo;
        const bool overflow = lo < (m_lo | other.m_lo);
        const half_t hi = m_hi + other.m_hi + overflow;
        return uint_pow2_t(lo, hi);
    }
    
    /// \brief Addition assignment.
    uint_pow2_t& operator+=(const uint_pow2_t& other) {
        *this = *this + other;
        return *this;
    }
    
    /// \brief Subtraction.
    constexpr uint_pow2_t operator-(const uint_pow2_t& other) const {
        return *this + (-other);
    }
    
    /// \brief Subtraction assignment.
    uint_pow2_t& operator-=(const uint_pow2_t& other) {
        *this = *this - other;
        return *this;
    }

private:
    static constexpr quater_t qlo(const half_t& x) { return (quater_t)x; }
    static constexpr quater_t qhi(const half_t& x) { return (quater_t)(x >> QUARTER_BITS); }

public:
    /// \brief Multiplication.
    constexpr uint_pow2_t operator*(const uint_pow2_t& other) const {
        if(*this == 0 || other == 0) {
            return uint_pow2_t(0);
        } else if(other == 1) {
            return *this;
        } else if(*this == 1) {
            return other;
        }
        
        // adapted from Github: calccrypto/uint256_t (MIT license)
        
        // split values into quarters parts
        half_t top[4] = { qhi(m_hi), qlo(m_hi), qhi(m_lo), qlo(m_lo)};
        half_t bottom[4] = { qhi(other.m_hi), qlo(other.m_hi), qhi(other.m_lo), qlo(other.m_lo)};
        half_t products[4][4];

        // multiply each component of the values
        for(int y = 3; y >= 0; y--){
            for(int x = 3; x >= 0; x--){
                products[3 - y][x] = top[x] * bottom[y];
            }
        }

        // first row
        half_t q3 = half_t(qlo(products[0][3]));
        half_t q2  = half_t(qlo(products[0][2])) + half_t(qhi(products[0][3]));
        half_t q1 = half_t(qlo(products[0][1])) + half_t(qhi(products[0][2]));
        half_t q0  = half_t(qlo(products[0][0])) + half_t(qhi(products[0][1]));

        // second row
        q2  += half_t(qlo(products[1][3]));
        q1 += half_t(qlo(products[1][2])) + half_t(qhi(products[1][3]));
        q0  += half_t(qlo(products[1][1])) + half_t(qhi(products[1][2]));

        // third row
        q1 += half_t(qlo(products[2][3]));
        q0  += half_t(qlo(products[2][2])) + half_t(qhi(products[2][3]));

        // fourth row
        q0  += half_t(qlo(products[3][3]));

        // combines the values, taking care of carry over
        return uint_pow2_t(0, q0 << QUARTER_BITS) +
               uint_pow2_t(q2 << QUARTER_BITS, qhi(q2)) +
               uint_pow2_t(0, q1) +
               uint_pow2_t(q3);
    }

    /// \brief Multiplication assignment.
    uint_pow2_t& operator*=(const uint_pow2_t& other) {
        *this = *this * other;
        return *this;
    }

private:
    static constexpr std::pair<uint_pow2_t, uint_pow2_t> divmod(const uint_pow2_t& a, const uint_pow2_t& b) {
        // adapted from Github: calccrypto/uint256_t (MIT license)
        if (b == 0){
            throw std::domain_error("Error: division or modulo by 0");
        } else if (b == 1) {
            return std::pair<uint_pow2_t, uint_pow2_t>(a, 0);
        } else if (a == b){
            return std::pair<uint_pow2_t, uint_pow2_t>(1, 0);
        } else if (a == 0 || a < b){
            return std::pair<uint_pow2_t, uint_pow2_t>(0, a);
        }

        std::pair<uint_pow2_t, uint_pow2_t> qr(0, a);
        
        const size_t bits_a = math::ilog2_ceil(a);
        const size_t bits_b = math::ilog2_ceil(b);
        
        uint_pow2_t copyd = b << (bits_a - bits_b);
        uint_pow2_t adder = uint_pow2_t(1) << (bits_a - bits_b);
        
        if (copyd > qr.second){
            copyd >>= 1;
            adder >>= 1;
        }
        
        while (qr.second >= b){
            if (qr.second >= copyd) {
                qr.second -= copyd;
                qr.first |= adder;
            }
            copyd >>= 1;
            adder >>= 1;
        }
        return qr;
    }

public:
    /// \brief Division.
    constexpr uint_pow2_t operator/(const uint_pow2_t& other) const {
        auto qr = divmod(*this, other);
        return qr.first;
    }

    /// \brief Division assignment.
    uint_pow2_t& operator/=(const uint_pow2_t& other) {
        *this = *this / other;
        return *this;
    }

    /// \brief Modulo.
    constexpr uint_pow2_t operator%(const uint_pow2_t& other) const {
        auto qr = divmod(*this, other);
        return qr.second;
    }

    /// \brief Modulo assignment.
    uint_pow2_t& operator%=(const uint_pow2_t& other) {
        *this = *this % other;
        return *this;
    }

    /// \brief Bitwise OR.
    constexpr uint_pow2_t operator|(const uint_pow2_t& other) const {
        return uint_pow2_t(m_lo | other.m_lo, m_hi | other.m_hi);
    }

    /// \brief Bitwise OR assignment.
    uint_pow2_t& operator|=(const uint_pow2_t& other) {
        *this = *this | other;
        return *this;
    }

    /// \brief Bitwise negation.
    constexpr uint_pow2_t operator~() const {
        return uint_pow2_t(~m_lo, ~m_hi);
    }

    /// \brief Bitwise AND.
    constexpr uint_pow2_t operator&(const uint_pow2_t& other) const {
        return uint_pow2_t(m_lo & other.m_lo, m_hi & other.m_hi);
    }

    /// \brief Bitwise AND assignment.
    uint_pow2_t& operator&=(const uint_pow2_t& other) {
        *this = *this & other;
        return *this;
    }

    /// \brief Bitwise XOR.
    constexpr uint_pow2_t operator^(const uint_pow2_t& other) const {
        return uint_pow2_t(m_lo ^ other.m_lo, m_hi ^ other.m_hi);
    }

    /// \brief Bitwise XOR assignment.
    uint_pow2_t& operator^=(const uint_pow2_t& other) {
        *this = *this ^ other;
        return *this;
    }
    
    /// \brief Left shift.
    constexpr uint_pow2_t operator<<(const uint64_t& lsh) const {
        if(lsh == 0) {
            return *this;
        } else if(lsh < HALF_BITS) {
            const half_t lo_mask = ~(HALF_MAX >> lsh);
            const uint64_t lo_rsh = (uint64_t)HALF_BITS - lsh;
            return uint_pow2_t(m_lo << lsh, m_hi << lsh | ((m_lo & lo_mask) >> lo_rsh));
        } else {
            return uint_pow2_t(0, m_lo << (lsh - HALF_BITS));
        }
        
    }

    /// \brief Left shift assignment.
    uint_pow2_t& operator<<=(const uint64_t& lsh) {
        *this = *this << lsh;
        return *this;
    }

    /// \brief Right shift.
    constexpr uint_pow2_t operator>>(const uint64_t& rsh) const {
        if(rsh == 0) {
            return *this;
        } else if(rsh < HALF_BITS) {
            const half_t hi_mask = HALF_MAX >> (HALF_BITS - rsh);
            const uint64_t hi_lsh = (uint64_t)HALF_BITS - rsh;
            return uint_pow2_t(m_lo >> rsh | ((m_hi & hi_mask) << hi_lsh), m_hi >> rsh);
        } else {
            return uint_pow2_t(m_hi >> (rsh - HALF_BITS), 0);
        }
    }
    
    /// \brief Right shift assignment.
    uint_pow2_t& operator>>=(const uint64_t& rsh) {
        *this = *this >> rsh;
        return *this;
    }
    
    constexpr uint_pow2_t operator<<(const uint_pow2_t& lsh) const { return *this << (uint64_t)lsh; } // if the shift operand doesn't fit into 64 bits, then it doesn't really matter anyway...
    uint_pow2_t& operator<<=(const uint_pow2_t& lsh) const { return (*this <<= (uint64_t)lsh); }
    constexpr uint_pow2_t operator>>(const uint_pow2_t& rsh) const { return *this >> (uint64_t)rsh; }
    uint_pow2_t& operator>>=(const uint_pow2_t& rsh) const { return (*this >>= (uint64_t)rsh); }

    /// \brief Equality check.
    constexpr bool operator==(const uint_pow2_t& other) const {
        return (m_lo == other.m_lo) && (m_hi == other.m_hi);
    }

    /// \brief Non-equality check.
    constexpr bool operator!=(const uint_pow2_t& other) const {
        return (m_lo != other.m_lo) || (m_hi != other.m_hi);
    }

    /// \brief Less-than check.
    constexpr bool operator<(const uint_pow2_t& other) const {
        return (m_hi < other.m_hi) || (m_hi == other.m_hi && m_lo < other.m_lo);
    }

    /// \brief Less-than-or-equal check.
    constexpr bool operator<=(const uint_pow2_t& other) const {
        return (m_hi < other.m_hi) || (m_hi == other.m_hi && m_lo <= other.m_lo);
    }

    /// \brief Greater-than check.
    constexpr bool operator>(const uint_pow2_t& other) const {
        return (m_hi > other.m_hi) || (m_hi == other.m_hi && m_lo > other.m_lo);
    }

    /// \brief Greater-than-or-equal check.
    constexpr bool operator>=(const uint_pow2_t& other) const {
        return (m_hi > other.m_hi) || (m_hi == other.m_hi && m_lo >= other.m_lo);
    }
    
    /// \cond INTERNAL
    constexpr size_t lzcnt() const {
        const size_t hi = intrisics::lzcnt0((half_t)(*this >> HALF_BITS));
        return hi == HALF_BITS ? HALF_BITS + intrisics::lzcnt0((half_t)*this) : hi;
    }
    
    constexpr size_t popcnt() const {
        return intrisics::popcnt((half_t)*this) + intrisics::popcnt((half_t)(*this >> HALF_BITS));
    }
    
    constexpr size_t tzcnt() const {
        const size_t lo = intrisics::tzcnt0((half_t)*this);
        return lo == HALF_BITS ? HALF_BITS + intrisics::tzcnt0((half_t)(*this >> HALF_BITS)) : lo;
    }
    
    std::ostream& outp(std::ostream& out) const {
        return print_uint(out, *this);
    }
    /// \endcond
} __attribute__((__packed__));

} // namespace tdc