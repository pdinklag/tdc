#pragma once

#include <iostream>
#include <utility>

#include <tdc/io/buffered_writer.hpp>

#include "factor.hpp"

namespace tdc {
namespace comp {
namespace lz77 {

class FactorFileOutput {
private:
    io::BufferedWriter<Factor>* out_;

public:
    inline FactorFileOutput(io::BufferedWriter<Factor>& out) : out_(&out) {
    }

    FactorFileOutput(const FactorFileOutput&) = default;
    FactorFileOutput(FactorFileOutput&&) = default;
    FactorFileOutput& operator=(const FactorFileOutput&) = default;
    FactorFileOutput& operator=(FactorFileOutput&&) = default;

    void emplace_back(const char_t literal) {
        out_->write(Factor(literal));
    }

    void emplace_back(const index_t src, const index_t len) {
        out_->write(Factor(src, len));
    }

    void emplace_back(Factor&& f) {
        out_->write(f);
    }

    void emplace_back(const Factor& f) {
        out_->write(f);
    }
};

}}}
