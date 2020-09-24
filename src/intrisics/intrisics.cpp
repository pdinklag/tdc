#include <tdc/intrisics/parallel_bits.hpp>
#include <tdc/intrisics/parallel_compare.hpp>

#include <immintrin.h>
#include <mmintrin.h>
#include <nmmintrin.h>

#ifdef __BMI2__
uint64_t tdc::intrisics::pext(const uint64_t x, const uint64_t mask) {
    return _pext_u64(x, mask);
}
#endif

// nb: this function CANNOT be inlined for whatever reason
uint64_t tdc::intrisics::pcmpgtub(const uint64_t a, const uint64_t b) {
    // _m_pcmpgtb is a comparison for SIGNED bytes, but we want an unsigned comparison
    // this is reached by XORing every value in our array with 0x80
    // this approach was micro-benchmarked against using an SSE max variant, as well as simple byte-wise comparison
    static constexpr __m64 XOR_MASK = (__m64)0x8080808080808080ULL;
    return (uint64_t)_m_pcmpgtb(
        _mm_xor_si64((__m64)a, XOR_MASK),
        _mm_xor_si64((__m64)b, XOR_MASK));
}
