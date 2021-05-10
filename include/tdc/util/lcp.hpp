#pragma once

namespace tdc {

/// \brief Computes the LCP array from the suffix array using Kasai's algorithm.
/// \tparam char_t the character type
/// \tparam idx_t the suffix/LCP array entry type
/// \param text the input text, assuming to be zero-terminated
/// \param n the length of the input text including the zero-terminator
/// \param sa the suffix array for the text
/// \param lcp the output LCP array
/// \param plcp an array of size n used as working space -- contains the permuted LCP array after the operation
template<typename char_t, typename idx_t>
static void lcp_kasai(const char_t* text, const size_t n, const idx_t* sa, idx_t* lcp, idx_t* plcp) {
    // compute phi array
    {
        for(size_t i = 1, prev = sa[0]; i < n; i++) {
            plcp[sa[i]] = prev;
            prev = sa[i];
        }
        plcp[sa[0]] = sa[n-1];
    }
    
    // compute PLCP array
    {
        for(size_t i = 0, l = 0; i < n - 1; ++i) {
            const auto phi_i = plcp[i];
            while(text[i + l] == text[phi_i + l]) ++l;
            plcp[i] = l;
            if(l) --l;
        }
    }
    
    // compute LCP array
    {
        lcp[0] = 0;
        for(size_t i = 1; i < n; i++) {
            lcp[i] = plcp[sa[i]];
        }
    }
}

} // namespace tdc
