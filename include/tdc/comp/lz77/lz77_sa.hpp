#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <sstream>

#include <divsufsort.h>

#include <tdc/util/lcp.hpp>
#include <tdc/util/literals.hpp>

namespace tdc {
namespace comp {
namespace lz77 {

class LZ77SA {
private:
    size_t m_threshold;

public:
    LZ77SA(size_t threshold = 2) : m_threshold(threshold) {
    }

    template<typename FactorOutput>
    void compress(std::istream& in, FactorOutput& out) {
        // read input fully
        std::string text(std::istreambuf_iterator<char>(in), {});
        text.push_back(0); // append sentinel
        
        // construct suffix and LCP array
        const size_t n = text.length();
        
        auto* sa = new saidx_t[n];
        
        auto result = divsufsort((const sauchar_t*)text.data(), sa, (saidx_t)n);
        
        auto* lcp = new saidx_t[n];
        auto* isa = new saidx_t[n];

        lcp_kasai(text.data(), n, sa, lcp, isa); // we're using ISA as a working array here, it won't actually contain the ISA after this
        
        // construct ISA
        for(size_t i = 0; i < n; i++) {
            isa[sa[i]] = i;
        }
        
        // factorize
        for(size_t i = 0; i + 1 < n;) {
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
                assert(max_pos >= 0);
                
                // output reference
                out.emplace_back(max_pos, max_lcp);
                i += max_lcp; //advance
            } else {
                // output literal
                out.emplace_back(text[i]);
                ++i; //advance
            }
        }
        
        // clean up
        delete[] isa;
        delete[] lcp;
        delete[] sa;
    }
    
    template<typename StatLogger>
    void log_stats(StatLogger& logger) {
    }
};

}}} // namespace tdc::comp::lz77
