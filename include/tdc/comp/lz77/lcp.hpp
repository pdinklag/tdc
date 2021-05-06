#pragma once

namespace tdc {
namespace comp {
namespace lz77 {

template<typename char_t, typename saidx_t>
static void compute_lcp(const char_t* buffer, const size_t n, const saidx_t* sa, saidx_t* lcp, saidx_t* work) {
    // compute phi array
    {
        for(size_t i = 1, prev = sa[0]; i < n; i++) {
            work[sa[i]] = prev;
            prev = sa[i];
        }
        work[sa[0]] = sa[n-1];
    }
    
    // compute PLCP array
    {
        for(size_t i = 0, l = 0; i < n - 1; ++i) {
            const auto phi_i = work[i];
            while(buffer[i + l] == buffer[phi_i + l]) ++l;
            work[i] = l;
            if(l) --l;
        }
    }
    
    // compute LCP array
    {
        lcp[0] = 0;
        for(size_t i = 1; i < n; i++) {
            lcp[i] = work[sa[i]];
        }
    }
}

}}} // namespace tdc::comp::lz77
