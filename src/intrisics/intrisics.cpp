#include <tdc/intrisics/pext.hpp>
#include <tdc/intrisics/pcmp.hpp>
#include <tdc/intrisics/popcnt.hpp>

#include <immintrin.h>
#include <mmintrin.h>
#include <nmmintrin.h>

#include <tdc/uint/uint40.hpp>
#include <tdc/uint/uint128.hpp>
#include <tdc/uint/uint256.hpp>

#ifdef __BMI2__
template<> uint64_t tdc::intrisics::pext(const uint64_t x, const uint64_t mask) {
    return _pext_u64(x, mask);
}

template<> uint8_t tdc::intrisics::pext(const uint8_t x, const uint8_t mask) { return tdc::intrisics::pext((uint64_t)x, (uint64_t)mask); }
template<> uint16_t tdc::intrisics::pext(const uint16_t x, const uint16_t mask) { return tdc::intrisics::pext((uint64_t)x, (uint64_t)mask); }
template<> uint32_t tdc::intrisics::pext(const uint32_t x, const uint32_t mask) { return tdc::intrisics::pext((uint64_t)x, (uint64_t)mask); }
template<> uint40_t tdc::intrisics::pext(const uint40_t x, const uint40_t mask) { return tdc::intrisics::pext((uint64_t)x, (uint64_t)mask); }

template<> uint128_t tdc::intrisics::pext(const uint128_t x, const uint128_t mask) {
    const size_t lo_cnt = popcnt((uint64_t)mask);
    const uint64_t pext_lo = _pext_u64(x, mask);
    const uint64_t pext_hi = _pext_u64(x >> 64, mask >> 64);
    return (uint128_t)pext_hi << lo_cnt | pext_lo;
    
    // TODO: benchmark against naive approach:
    /*
    uint128_t check = 1;
    uint128_t result = 0;
    uint128_t result_lsh = 0;
    for(size_t i = 0; i < 128; i++) {
        if(mask & check) {
            const auto bit = (x & check) >> i;
            result |= bit << result_lsh++;
        }
        check <<= 1;
    }
    return result;
    */
}

template<> uint256_t tdc::intrisics::pext(const uint256_t x, const uint256_t mask) {
    const size_t lo_cnt = popcnt((uint128_t)mask);
    const uint128_t pext_lo = pext((uint128_t)x, (uint128_t)mask);
    const uint128_t pext_hi = pext((uint128_t)(x >> 128), (uint128_t)(mask >> 128));
    return (uint256_t)pext_hi << lo_cnt | pext_lo;
    // TODO: benchmark against naive approach
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
