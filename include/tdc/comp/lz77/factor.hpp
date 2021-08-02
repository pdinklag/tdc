#pragma once

#include <vector>

#include <tdc/util/char.hpp>
#include <tdc/util/index.hpp>

namespace tdc {
namespace comp {
namespace lz77 {

struct Factor {
    index_t src;
    index_t len;

    inline Factor() : src(0), len(0) {
    }

    inline Factor(const char_t _literal) : src(_literal), len(0) {
    }

    inline Factor(const index_t _src, const index_t _len) : src(_src), len(_len) {
    }

    Factor(const Factor&) = default;
    Factor(Factor&&) = default;
    Factor& operator=(const Factor&) = default;
    Factor& operator=(Factor&&) = default;

    inline bool is_reference() const {
        return len > 0;
    }

    inline bool is_literal() const {
        return !is_reference();
    }

    inline char_t literal() const {
        return (char_t)src;
    }
};

}}}
