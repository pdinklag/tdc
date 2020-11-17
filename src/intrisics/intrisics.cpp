#include <tdc/intrisics/pext.hpp>
#include <tdc/intrisics/pcmp.hpp>
#include <tdc/intrisics/popcnt.hpp>

#include <immintrin.h>
#include <mmintrin.h>
#include <nmmintrin.h>

#include <tdc/uint/uint40.hpp>
#include <tdc/uint/uint1024.hpp>

#ifdef __BMI2__
template<> uint64_t tdc::intrisics::pext(const uint64_t& x, const uint64_t& mask) {
    return _pext_u64(x, mask);
}

template<> uint8_t tdc::intrisics::pext(const uint8_t& x, const uint8_t& mask) { return tdc::intrisics::pext((uint64_t)x, (uint64_t)mask); }
template<> uint16_t tdc::intrisics::pext(const uint16_t& x, const uint16_t& mask) { return tdc::intrisics::pext((uint64_t)x, (uint64_t)mask); }
template<> uint32_t tdc::intrisics::pext(const uint32_t& x, const uint32_t& mask) { return tdc::intrisics::pext((uint64_t)x, (uint64_t)mask); }
template<> uint40_t tdc::intrisics::pext(const uint40_t& x, const uint40_t& mask) { return tdc::intrisics::pext((uint64_t)x, (uint64_t)mask); }

template<> uint128_t tdc::intrisics::pext(const uint128_t& x, const uint128_t& mask) {
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

template<> uint256_t tdc::intrisics::pext(const uint256_t& x, const uint256_t& mask) {
    const size_t lo_cnt = popcnt((uint128_t)mask);
    const uint128_t pext_lo = pext((uint128_t)x, (uint128_t)mask);
    const uint128_t pext_hi = pext((uint128_t)(x >> 128), (uint128_t)(mask >> 128));
    return (uint256_t)pext_hi << lo_cnt | (uint256_t)pext_lo;
    // TODO: benchmark against naive approach
}

template<> uint512_t tdc::intrisics::pext(const uint512_t& x, const uint512_t& mask) {
    const size_t lo_cnt = popcnt((uint256_t)mask);
    const uint256_t pext_lo = pext((uint256_t)x, (uint256_t)mask);
    const uint256_t pext_hi = pext((uint256_t)(x >> 256), (uint256_t)(mask >> 256));
    return (uint512_t)pext_hi << lo_cnt | (uint512_t)pext_lo;
    // TODO: benchmark against naive approach
}

template<> uint1024_t tdc::intrisics::pext(const uint1024_t& x, const uint1024_t& mask) {
    const size_t lo_cnt = popcnt((uint512_t)mask);
    const uint512_t pext_lo = pext((uint512_t)x, (uint512_t)mask);
    const uint512_t pext_hi = pext((uint512_t)(x >> 512), (uint512_t)(mask >> 512));
    return (uint1024_t)pext_hi << lo_cnt | (uint1024_t)pext_lo;
    // TODO: benchmark against naive approach
}
#endif

template<>
uint64_t tdc::intrisics::pcmpgtu<uint64_t, uint8_t>(const uint64_t& a, const uint64_t& b) {
    // _m_pcmpgtb is a comparison for SIGNED bytes, but we want an unsigned comparison
    // this is reached by XORing every value in our array with 0x80
    // this approach was micro-benchmarked against using an SSE max variant, as well as simple byte-wise comparison
    static constexpr __m64 XOR_MASK_64 = (__m64)0x80'80'80'80'80'80'80'80ULL;
    return (uint64_t)_m_pcmpgtb(
        _mm_xor_si64((__m64)a, XOR_MASK_64),
        _mm_xor_si64((__m64)b, XOR_MASK_64));
}

template<>
uint256_t tdc::intrisics::pcmpgtu<uint256_t, uint16_t>(const uint256_t& a, const uint256_t& b) {
    // the instruction we use is a comparison for SIGNED bytes, but we want an unsigned comparison
    // this is reached by XORing every value in our array with 0x80
    // this approach was micro-benchmarked against using an SSE max variant, as well as simple byte-wise comparison
    static constexpr uint256_t XOR_MASK_256 = uint256_t(0x8000'8000'8000'8000ULL, 0x8000'8000'8000'8000ULL, 0x8000'8000'8000'8000ULL, 0x8000'8000'8000'8000ULL);
    
    uint256_t a_signed = a ^ XOR_MASK_256;
    uint256_t b_signed = b ^ XOR_MASK_256;
    
    const uint64_t* pa = (const uint64_t*)&a_signed;
    __m256i _a = _mm256_set_epi64x(pa[0], pa[1], pa[2], pa[3]);
    const uint64_t* pb = (const uint64_t*)&b_signed;
    __m256i _b = _mm256_set_epi64x(pb[0], pb[1], pb[2], pb[3]);
    _a = _mm256_cmpgt_epi16(_a, _b);
    
    uint64_t buf[4];
    _mm256_storeu_si256((__m256i*)&buf, _a);
    return uint256_t(buf[3], buf[2], buf[1], buf[0]);
}

template<>
uint1024_t tdc::intrisics::pcmpgtu<uint1024_t, uint32_t>(const uint1024_t& a, const uint1024_t& b) {
    uint32_t result[32];
    const uint32_t* _a = (const uint32_t*)&a;
    const uint32_t* _b = (const uint32_t*)&b;
    for(size_t i = 0; i < 32; i++) {
        result[i] = -(*_a++ > *_b++);
    }
    return *((uint1024_t*)result);
}
