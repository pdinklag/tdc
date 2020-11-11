
#pragma once

#include <limits>
#include <string>
#include <utility>

#ifdef MPFR_FOUND

#include <mpfr.h>

namespace tdc {
namespace mpf {

/// \brief The precision for \ref mpfloat_t to simulate \c double.
constexpr mpfr_prec_t DOUBLE_PRECISION = 53;

/// \brief Multi-precision floating point values.
///
/// In the following, we simply refer to these as \em mpfloats.
/// 
/// This is a C++ binding to MPFR's \c mpfr_t type.
/// Beware that because of how MPFR works, every instance will cause allocations.
/// In complex arithmetics, this may cause an excessive amount of allocations of temporary instances.
///
/// If the number of allocations is critical, declare instances beforehand and re-use them using setters and
/// assignment operators rather than the \c const functions.
///
/// \tparam m_precision the precision of the floating point value, i.e., the number of bits of its mantissa.
/// \tparam m_rnd the rounding to use for operations, using \em nearest as the default setting. Refer to the MPFR documentation for details.
template<mpfr_prec_t m_precision, mpfr_rnd_t m_rnd = MPFR_RNDN>
class mpfloat_t {
public:
    /// \brief Constructs an mpfloat that equals PI.
    static mpfloat_t pi() {
        mpfloat_t r;
        mpfr_const_pi(r.m_value, m_rnd);
        return r;
    }

    /// \brief Computes the square root of an mpfloat in a new mpfloat.
    static mpfloat_t sqrt(const mpfloat_t& x) {
        mpfloat_t r(x);
        return r.sqrt();
    }

    /// \brief Computes the natural logarithm of an mpfloat in a new mpfloat.
    static mpfloat_t log(const mpfloat_t& x) {
        mpfloat_t r(x);
        return r.log();
    }

    /// \brief Computes Euler's number to an mpfloat's power in a new mpfloat.
    static mpfloat_t exp(const mpfloat_t& x) {
        mpfloat_t r(x);
        return r.exp();
    }

    /// \brief Computes the natural logarithm of the absolute value of the gamma function of an mpfloat in a new mpfloat.
    static mpfloat_t lgamma(const mpfloat_t& x) {
        mpfloat_t r(x);
        return r.lgamma();
    }

    /// \brief Rounds an mpfloat up to the next integer in a new mpfloat.
    static mpfloat_t ceil(const mpfloat_t& x) {
        mpfloat_t r(x);
        return r.ceil();
    }

    /// \brief Rounds an mpfloat down to the previous integer in a new mpfloat.
    static mpfloat_t floor(const mpfloat_t& x) {
        mpfloat_t r(x);
        return r.floor();
    }

    /// \brief Rounds an float to the nearest integer in a new float.
    static mpfloat_t round(const mpfloat_t& x) {
        mpfloat_t r(x);
        return r.round();
    }

    /// \brief Returns the minimum of two mpfloats in a new mpfloat.
    static mpfloat_t min(const mpfloat_t& a, const mpfloat_t& b) {
        mpfloat_t r(a);
        return r.min(b);
    }

    /// \brief Returns the maximum of two mpfloats in a new mpfloat.
    static mpfloat_t max(const mpfloat_t& a, const mpfloat_t& b) {
        mpfloat_t r(a);
        return r.max(b);
    }

    /// \brief Returns the absolute value of a mpfloat in a new mpfloat.
    static mpfloat_t abs(const mpfloat_t& x) {
        mpfloat_t r(x);
        return r.abs();
    }

private:
    mpfr_t m_value;
    
public:
    /// \brief Constructs an mpfloat with NaN value.
    mpfloat_t() {
        mpfr_init2(m_value, m_precision);
    }

    /// \brief Constructs an mpfloat from a double.
    mpfloat_t(const double d) : mpfloat_t() {
        *this = d;
    }

    /// \brief Destructor.
    ~mpfloat_t() {
        if(m_value[0]._mpfr_d) {
            mpfr_clear(m_value);
        }
    }

    mpfloat_t(const mpfloat_t& other) : mpfloat_t() {
        *this = other;
    }
    
    mpfloat_t(mpfloat_t&& other) : mpfloat_t() {
        *this = std::move(other);
    }

    /// \brief Assigns a double value to this mpfloat.
    mpfloat_t& operator=(const double d) {
        mpfr_set_d(m_value, d, m_rnd);
        return *this;
    }

    /// \brief Assigns an integer value to this mpfloat.
    template<typename int_t>
    mpfloat_t& operator=(const int_t& x) {
        static_assert(std::numeric_limits<int_t>::is_integer);
        
        // TODO: support arbitrary-width integer types
        mpfr_set_uj(m_value, x, m_rnd);
        return *this;
    }

    mpfloat_t& operator=(const mpfloat_t& other) {
        mpfr_set(m_value, other.m_value, m_rnd);
        return *this;
    }
    
    mpfloat_t& operator=(mpfloat_t&& other) {
        mpfr_set(m_value, other.m_value, m_rnd);
        mpfr_clear(other.m_value);
        return *this;
    }

    /// \brief Reports the precision of this mpfloat.
    mpfr_prec_t precision() const {
        return m_precision;
    }

    /// \brief Provides access to the underlying \c mpfr_t for advanced operations.
    mpfr_t& mpfr() {
        return m_value;
    }

    /// \brief Converts this mpfloat to a double.
    explicit operator double() const {
        return mpfr_get_d(m_value, m_rnd);
    }

    /// \brief Rounds this mpfloat to the nearest integer.
    template<typename int_t>
    explicit operator int_t() const {
        static_assert(std::numeric_limits<int_t>::is_integer);
        // TODO: support arbitrary-width integer types
        return mpfr_get_uj(m_value, m_rnd);
    }

    /// \brief Negates this mpfloat in a new mpfloat.
    ///
    /// Use \ref neg to negate this mpfloat without allocation.
    mpfloat_t operator-() const {
        mpfloat_t r(*this);
        return r.neg();
    }

    /// \brief Addition assignment.
    mpfloat_t& operator+=(const mpfloat_t& other) {
        mpfr_add(m_value, m_value, other.m_value, m_rnd);
        return *this;
    }

    /// \brief Addition assignment.
    mpfloat_t& operator+=(const double d) {
        mpfr_add_d(m_value, m_value, d, m_rnd);
        return *this;
    }

    /// \brief Addition in a new mpfloat.
    mpfloat_t operator+(const mpfloat_t& other) const {
        mpfloat_t r(*this);
        r += other;
        return r;
    }

    /// \brief Addition in a new mpfloat.
    mpfloat_t operator+(const double d) const {
        mpfloat_t r(*this);
        r += d;
        return r;
    }

    /// \brief Subtraction assignment.
    mpfloat_t& operator-=(const mpfloat_t& other) {
        mpfr_sub(m_value, m_value, other.m_value, m_rnd);
        return *this;
    }

    /// \brief Subtraction assignment.
    mpfloat_t& operator-=(const double d) {
        mpfr_sub_d(m_value, m_value, d, m_rnd);
        return *this;
    }

    /// \brief Subtraction in a new mpfloat.
    mpfloat_t operator-(const mpfloat_t& other) const {
        mpfloat_t r(*this);
        r -= other;
        return r;
    }

    /// \brief Subtraction in a new mpfloat.
    mpfloat_t operator-(const double d) const {
        mpfloat_t r(*this);
        r -= d;
        return r;
    }

    /// \brief Multiplication assignment.
    mpfloat_t& operator*=(const mpfloat_t& other) {
        mpfr_mul(m_value, m_value, other.m_value, m_rnd);
        return *this;
    }

    /// \brief Multiplication assignment.
    mpfloat_t& operator*=(const double d) {
        mpfr_mul_d(m_value, m_value, d, m_rnd);
        return *this;
    }

    /// \brief Multiplication in a new mpfloat.
    mpfloat_t operator*(const mpfloat_t& other) const {
        mpfloat_t r(*this);
        r *= other;
        return r;
    }

    /// \brief Multiplication in a new mpfloat.
    mpfloat_t operator*(const double d) const {
        mpfloat_t r(*this);
        r *= d;
        return r;
    }

    /// \brief Division assignment.
    mpfloat_t& operator/=(const mpfloat_t& other) {
        mpfr_div(m_value, m_value, other.m_value, m_rnd);
        return *this;
    }

    /// \brief Division assignment.
    mpfloat_t& operator/=(const double d) {
        mpfr_div_d(m_value, m_value, d, m_rnd);
        return *this;
    }

    /// \brief Division in a new mpfloat.
    mpfloat_t operator/(const mpfloat_t& other) const {
        mpfloat_t r(*this);
        r /= other;
        return r;
    }

    /// \brief Division in a new mpfloat.
    mpfloat_t operator/(const double d) const {
        mpfloat_t r(*this);
        r /= d;
        return r;
    }

    /// \brief Assigns the negation of this mpfloat.
    mpfloat_t& neg() {
        mpfr_neg(m_value, m_value, m_rnd);
        return *this;
    }

    /// \brief Assigns the absolute value of this mpfloat.
    mpfloat_t& abs() {
        mpfr_abs(m_value, m_value, m_rnd);
        return *this;
    }

    /// \brief Assigns the square root of this mpfloat.
    mpfloat_t& sqrt() {
        mpfr_sqrt(m_value, m_value, m_rnd);
        return *this;
    }

    /// \brief Assigns the natural logarithm of this mpfloat.
    mpfloat_t& log() {
        mpfr_log(m_value, m_value, m_rnd);
        return *this;
    }

    /// \brief Assigns Euler's number to the mpfloat's power.
    mpfloat_t& exp() {
        mpfr_exp(m_value, m_value, m_rnd);
        return *this;
    }

    /// \brief Assigns the natural logarithm of the absolute value of the gamma function for this mpfloat.
    /// \param sign Will be assigned the sign of the gamma function for this mpfloat.
    mpfloat_t& lgamma(int* sign) {
        mpfr_lgamma(m_value, sign, m_value, m_rnd);
        return *this;
    }

    /// \brief Assigns the natural logarithm of the absolute value of the gamma function for this mpfloat.
    mpfloat_t& lgamma() {
        int sign;
        return lgamma(&sign);
    }

    /// \brief Assigns the mpfloat's value rounded down to the previous integer.
    mpfloat_t& floor() {
        mpfr_floor(m_value, m_value);
        return *this;
    }

    /// \brief Assigns the mpfloat's value rounded up to the next integer.
    mpfloat_t& ceil() {
        mpfr_ceil(m_value, m_value);
        return *this;
    }

    /// \brief Assigns the mpfloat's value rounded to the nearest integer.
    mpfloat_t& round() {
        mpfr_round(m_value, m_value);
        return *this;
    }

    /// \brief Assigns the minimum of this' and another mpfloat's values.
    mpfloat_t& min(const mpfloat_t& other) {
        mpfr_min(m_value, m_value, other.m_value, m_rnd);
        return *this;
    }

    /// \brief Assigns the maximum of this' and another mpfloat's values.
    mpfloat_t& max(const mpfloat_t& other) {
        mpfr_max(m_value, m_value, other.m_value, m_rnd);
        return *this;
    }

    /// \brief Assigns the difference between a double value and this mpfloat's value.
    mpfloat_t& subtract_from(const double d) {
        mpfr_d_sub(m_value, d, m_value, m_rnd);
        return *this;
    }

    /// \brief Assigns the quotient of a double value divided by this mpfloat's value.
    mpfloat_t& divide(const double d) {
        mpfr_d_div(m_value, d, m_value, m_rnd);
        return *this;
    }

    /// \brief Equality comparison.
    bool operator==(const mpfloat_t& other) const {
        return mpfr_equal_p(m_value, other.m_value) != 0;
    }

    /// \brief Inequality comparison.
    bool operator!=(const mpfloat_t& other) const {
        return mpfr_equal_p(m_value, other.m_value) == 0;
    }

    /// \brief Less-than comparison.
    bool operator<(const mpfloat_t& other) const {
        return mpfr_less_p(m_value, other.m_value) != 0;
    }

    /// \brief Less-than-or-equal comparison.
    bool operator<=(const mpfloat_t& other) const {
        return mpfr_lessequal_p(m_value, other.m_value) != 0;
    }

    /// \brief Greater-than comparison.
    bool operator>(const mpfloat_t& other) const {
        return mpfr_greater_p(m_value, other.m_value) != 0;
    }

    /// \brief Greater-than-or-equal comparison.
    bool operator>=(const mpfloat_t& other) const {
        return mpfr_greaterequal_p(m_value, other.m_value) != 0;
    }

    /// \brief Equality comparison.
    bool operator==(const double d) const {
        return mpfr_cmp_d(m_value, d) == 0;
    }

    /// \brief Inequality comparison.
    bool operator!=(const double d) const {
        return mpfr_cmp_d(m_value, d) != 0;
    }

    /// \brief Less-than comparison.
    bool operator<(const double d) const {
        return mpfr_cmp_d(m_value, d) < 0;
    }

    /// \brief Less-than-or-equal comparison.
    bool operator<=(const double d) const {
        return mpfr_cmp_d(m_value, d) <= 0;
    }

    /// \brief Greater-than comparison.
    bool operator>(const double d) const {
        return mpfr_cmp_d(m_value, d) > 0;
    }

    /// \brief Greater-than-or-equal comparison.
    bool operator>=(const double d) const {
        return mpfr_cmp_d(m_value, d) >= 0;
    }
};

}} // namespace tdc::mpf

/// \brief Addition in a new mpfloat.
template<mpfr_prec_t prec, mpfr_rnd_t rnd>
tdc::mpf::mpfloat_t<prec, rnd> operator+(const double d, const tdc::mpf::mpfloat_t<prec, rnd>& f) {
    return f + d;
}

/// \brief Multiplication in a new mpfloat.
template<mpfr_prec_t prec, mpfr_rnd_t rnd>
tdc::mpf::mpfloat_t<prec, rnd> operator*(const double d, const tdc::mpf::mpfloat_t<prec, rnd>& f) {
    return f * d;
}

/// \brief Subtracts an mpfloat from a double in a new mpfloat.
template<mpfr_prec_t prec, mpfr_rnd_t rnd>
tdc::mpf::mpfloat_t<prec, rnd> operator-(const double d, const tdc::mpf::mpfloat_t<prec, rnd>& f) {
    tdc::mpf::mpfloat_t<prec, rnd> r(f);
    return r.subtract_from(d);
}

/// \brief Divides a double by an mpfloat in a new mpfloat.
template<mpfr_prec_t prec, mpfr_rnd_t rnd>
tdc::mpf::mpfloat_t<prec, rnd> operator/(const double d, const tdc::mpf::mpfloat_t<prec, rnd>& f) {
    tdc::mpf::mpfloat_t<prec, rnd> r(f);
    return r.divide(d);
}

#else

static_assert(false, "MPFR is not available");

#endif
