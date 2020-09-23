#include <tdc/pred/fusion_node_internals.hpp>

#include <immintrin.h>
#include <mmintrin.h>
#include <nmmintrin.h>

uint64_t tdc::pred::internal::compress(const uint64_t key, const uint64_t& mask) {
    #ifdef __BMI2__
    return _pext_u64(key, mask);
    #else
    static_assert(false, "BMI2 not supported (when building in Debug mode, you may have to pass -mbmi2 to the compiler)");
    // TODO: provide fallback implementation?
    #endif
}

uint64_t tdc::pred::internal::repeat(const uint8_t x) {
    static constexpr uint64_t REPEAT_MUL = 0x01'01'01'01'01'01'01'01ULL;
    return uint64_t(x) * REPEAT_MUL;
}

size_t tdc::pred::internal::rank(const uint64_t cx_repeat, const uint64_t array) {
    // compare it against the given array of 8 keys (in parallel)
    // note that _m_pcmpgtb is a comparison for SIGNED bytes, but we want an unsigned comparison
    // this is reached by XORing every value in our array with 0x80
    // this approach was micro-benchmarked against using an SSE max variant, as well as simple byte-wise comparison
    const uint64_t cmp = (uint64_t)_m_pcmpgtb(
        _mm_xor_si64((__m64)array, (__m64)0x8080808080808080ULL),
        _mm_xor_si64((__m64)cx_repeat, (__m64)0x8080808080808080ULL));
    
    // find the position of the first key greater than the compressed key
    // this is as easy as counting the trailing zeroes, of which there are a multiple of 8
    const size_t ctz = __builtin_ctzll(cmp) / 8;
    
    // the rank of our key is at the position before
    return ctz - 1;
}

size_t tdc::pred::internal::match(const uint64_t x, const uint64_t& mask, const uint64_t& branch, const uint64_t& free) {
    // compress the key and repeat it eight times
    const uint8_t cx = compress(x, mask);
    const uint64_t cx_repeat = repeat(cx);
    
    // construct our matching array by replacing all dontcares by the corresponding bits of the compressed key
    const uint64_t match_array = branch | (cx_repeat & free);
    
    // now find the rank of the key in that array
    return rank(cx_repeat, match_array);
}
