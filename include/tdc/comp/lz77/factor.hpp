#pragma once

#include <algorithm>
#include <vector>

#include <tdc/util/char.hpp>
#include <tdc/util/index.hpp>

namespace tdc {
namespace comp {
namespace lz77 {

struct Factor {
public:
    index_t src;
    index_t len;

    inline Factor() : src(INDEX_MAX), len(INDEX_MAX) {
    }

    inline Factor(const char_t _literal) : src(_literal), len(0) {
    }

    inline Factor(const index_t _src, const index_t _len) : src(_src), len(_len) {
    }

    Factor(const Factor&) = default;
    Factor(Factor&&) = default;
    Factor& operator=(const Factor&) = default;
    Factor& operator=(Factor&&) = default;

    inline bool is_valid() const {
        return len < INDEX_MAX;
    }

    inline bool is_reference() const {
        return len > 0;
    }

    inline bool is_literal() const {
        return !is_reference();
    }

    inline char_t literal() const {
        return (char_t)src;
    }

    inline size_t decoded_length() const {
        return std::max(size_t(1), (size_t)len);
    }
};

}}}
