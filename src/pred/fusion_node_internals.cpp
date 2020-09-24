#include <tdc/pred/fusion_node_internals.hpp>

using namespace tdc::pred::internal;

#include <immintrin.h>
#include <mmintrin.h>
#include <nmmintrin.h>

uint64_t tdc::pred::internal::pext(const uint64_t x, const uint64_t mask) {
    #ifdef __BMI2__
    return _pext_u64(x, mask);
    #else
    static_assert(false, "BMI2 not supported (when building in Debug mode, you may have to pass -mbmi2 to the compiler)");
    #endif
}

// nb: this function CANNOT be inlined for whatever reason
uint64_t tdc::pred::internal::pcmpgtub(const uint64_t a, const uint64_t b) {
    // _m_pcmpgtb is a comparison for SIGNED bytes, but we want an unsigned comparison
    // this is reached by XORing every value in our array with 0x80
    // this approach was micro-benchmarked against using an SSE max variant, as well as simple byte-wise comparison
    static constexpr __m64 XOR_MASK = (__m64)0x8080808080808080ULL;
    return (uint64_t)_m_pcmpgtb(
        _mm_xor_si64((__m64)a, XOR_MASK),
        _mm_xor_si64((__m64)b, XOR_MASK));
}

class FusionNodeInternals<8>;
