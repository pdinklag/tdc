#pragma once

#include <iostream>
#include <utility>

#include "factor.hpp"

namespace tdc {
namespace comp {
namespace lz77 {

class FactorReadableOutput {
private:
    static constexpr char REF_BEGIN[] = "(";
    static constexpr char REF_SEP[] = ",";
    static constexpr char REF_END[] = ")";

    std::ostream* out_;

    void print(Factor&& f) {
        if(f.is_reference()) {
            *out_ << REF_BEGIN << f.src << REF_SEP << f.len << REF_END;
        } else {
            *out_ << f.literal();
        }
    }

public:
    inline FactorReadableOutput(std::ostream& out) : out_(&out) {
    }

    FactorReadableOutput(const FactorReadableOutput&) = default;
    FactorReadableOutput(FactorReadableOutput&&) = default;
    FactorReadableOutput& operator=(const FactorReadableOutput&) = default;
    FactorReadableOutput& operator=(FactorReadableOutput&&) = default;

    void emplace_back(const char_t literal) {
        print(Factor(literal));
    }

    void emplace_back(const index_t src, const index_t len) {
        print(Factor(src, len));
    }

    void emplace_back(Factor&& f) {
        print(std::move(f));
    }

    void emplace_back(const Factor& f) {
        print(Factor(f));
    }
};

}}}
