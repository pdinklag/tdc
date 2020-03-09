#include <tdc/math/ilog2.hpp>

#include <tdc/math/isqrt.hpp>
#include <tdc/util/likely.hpp>

uint64_t tdc::math::isqrt_floor(const uint64_t x) {
    if(tdc_unlikely(x < 4ULL)) {
        return uint64_t(x > 0);
    }
    
    uint64_t e = ilog2_floor(x) >> 1;
    uint64_t r = 1ULL;

    while(e--) {
        const uint64_t cur = x >> (e << 1ULL);
        const uint64_t sm = r << 1ULL;
        const uint64_t lg = sm + 1ULL;
        r = (sm + (lg * lg <= cur));
    }
    return r;
}

uint64_t tdc::math::isqrt_ceil(const uint64_t x) {
    const uint64_t r = isqrt_floor(x);
    return r + (r * r < x);
}
