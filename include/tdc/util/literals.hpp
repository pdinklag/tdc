#pragma once

#include <cstddef>

/// \brief Custom literal for "kilo", multiplying a size by <tt>10^3</tt>.
constexpr size_t operator"" _K(unsigned long long s) { return s * 1'000ULL; }

/// \brief Custom literal for "mega", multiplying a size by <tt>10^6</tt>.
constexpr size_t operator"" _M(unsigned long long s) { return s * 1'000'000ULL; }

/// \brief Custom literal for "giga", multiplying a size by <tt>10^9</tt>.
constexpr size_t operator"" _G(unsigned long long s) { return s * 1'000'000'000ULL; }

/// \brief Custom literal for "kibi", multiplying a size by <tt>2^10</tt>.
constexpr size_t operator"" _Ki(unsigned long long s) { return s << 10ULL; }

/// \brief Custom literal for "mebi", multiplying a size by <tt>2^20</tt>.
constexpr size_t operator"" _Mi(unsigned long long s) { return s << 20ULL; }

/// \brief Custom literal for "gibi", multiplying a size by <tt>2^30</tt>.
constexpr size_t operator"" _Gi(unsigned long long s) { return s << 30ULL; }
