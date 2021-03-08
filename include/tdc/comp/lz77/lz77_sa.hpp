#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <sstream>

#include <divsufsort.h>

#include <tdc/util/literals.hpp>
#include "stats.hpp"

namespace tdc {
namespace comp {
namespace lz77 {

template<bool m_track_stats = false>
class LZ77SA {
private:
    Stats m_stats;
    size_t m_threshold;

public:
    LZ77SA(size_t threshold) : m_threshold(threshold) {
    }
    
    void compress(std::istream& in, std::ostream& out) {
        // read input fully
        std::string text(std::istreambuf_iterator<char>(in), {});
        
        // construct suffix and LCP array
        const size_t n = text.length();
        if constexpr(m_track_stats) m_stats.input_size = n;
        
        auto* sa = new saidx_t[n];
        auto* lcp = new saidx_t[n];
        
        auto result = divsuflcpsort((const sauchar_t*)text.data(), sa, lcp, (saidx_t)n);
        
        // construct ISA
        auto* isa = new saidx_t[n];
        for(size_t i = 0; i < n; i++) {
            isa[sa[i]] = i;
        }
        
        // factorize
        for(size_t i = 0; i+1 < n;) {
            // get SA position for suffix i
            const size_t cur_pos = isa[i];
            // assert(cur_pos > 0); // isa[i] == 0 <=> T[i] = 0

            // compute naively PSV
            // search "upwards" in LCP array
            // include current, exclude last
            size_t psv_lcp = lcp[cur_pos];
            ssize_t psv_pos = cur_pos - 1;
            if (psv_lcp > 0) {
                while (psv_pos >= 0 && sa[psv_pos] > sa[cur_pos]) {
                    psv_lcp = std::min<size_t>(psv_lcp, lcp[psv_pos--]);
                }
            }

            // compute naively NSV
            // search "downwards" in LCP array
            // exclude current, include last
            size_t nsv_lcp = 0;
            size_t nsv_pos = cur_pos + 1;
            if (nsv_pos < n) {
                nsv_lcp = SIZE_MAX;
                do {
                    nsv_lcp = std::min<size_t>(nsv_lcp, lcp[nsv_pos]);
                    if (sa[nsv_pos] < sa[cur_pos]) {
                        break;
                    }
                } while (++nsv_pos < n);

                if (nsv_pos >= n) {
                    nsv_lcp = 0;
                }
            }

            //select maximum
            const size_t max_lcp = std::max(psv_lcp, nsv_lcp);
            if(max_lcp >= m_threshold) {
                const ssize_t max_pos = (max_lcp == psv_lcp) ? psv_pos : nsv_pos;
                assert(max_pos < n);
                assert(max_pos > 0);
                
                // output reference
                out << "(" << sa[max_pos] << "," << max_lcp << ")";
                if constexpr(m_track_stats) ++m_stats.num_refs;

                i += max_lcp; //advance
            } else {
                // output literal
                out << text[i];
                if constexpr(m_track_stats) ++m_stats.num_literals;
                
                ++i; //advance
            }
        }
        
        // clean up
        delete[] isa;
        delete[] lcp;
        delete[] sa;
    }
    
    const Stats& stats() const { return m_stats; }
};

}}} // namespace tdc::comp::lz77
