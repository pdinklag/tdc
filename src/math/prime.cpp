#include <tdc/math/isqrt.hpp>
#include <tdc/math/prime.hpp>

#include <tdc/util/likely.hpp>

bool tdc::math::is_prime(const uint64_t p) {
    if(p % 2 == 0) {
        return false;
    } else {    
        const uint64_t m = isqrt_ceil(p);

        // first, check against small primes less than sqrt(p)
        size_t j = 2;
        uint64_t i = SMALL_PRIMES[j];
        while(i <= m && j < NUM_SMALL_PRIMES - 1) {
            if((p % i) == 0) return false;
            i = SMALL_PRIMES[++j];
        }

        // afterwards, use folklore algorithm for i less than sqrt(p)
        i = 5ULL + ((i - 5ULL) / 6ULL) * 6ULL;
        while(i <= m) {
            if(((p % i) == 0) || (p % (i+2) == 0)) return false;
            i += 6ULL;
        }
        return true;
    }
}

uint64_t tdc::math::prime_successor(const uint64_t p_) {
    uint64_t p = p_;
    
    if(tdc_unlikely(p == 0)) return 0;
    if(tdc_unlikely(p == 2)) return 2;
    if(p % 2 == 0) ++p; // all primes > 2 are odd

    // linear search - the gap between two primes is hopefully very low
    // in the worst case, because there must be a prime between p and 2p, this takes p steps
    while(!is_prime(p)) p += 2;
    return p;
}

uint64_t tdc::math::prime_predecessor(const uint64_t p_) {
    uint64_t p = p_;
    
    if(tdc_unlikely(p == 0)) return 0;
    if(tdc_unlikely(p == 2)) return 2;
    if(p % 2 == 0) --p; // all primes > 2 are odd

    // linear search - the gap between two primes is hopefully very low
    // in the worst case, because there must be a prime between p and 2p, this takes p steps
    while(!is_prime(p)) p -= 2;
    return p;
}
