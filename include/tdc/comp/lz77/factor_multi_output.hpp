#pragma once

#include <tuple>

namespace tdc {
namespace comp {
namespace lz77 {

template<typename... Outputs>
class FactorMultiOutput {
private:
    static constexpr std::size_t num_ = sizeof...(Outputs);
    std::tuple<Outputs*...> outputs_;

    template<size_t To = 0>
    void emplace_back_all(const index_t src, const index_t len) {
        if constexpr(To < num_) {
            get<To>().emplace_back(src, len);
            emplace_back_all<To+1>(src, len);
        }
    }

    template<size_t To = 0>
    void emplace_back_all(const char_t literal) {
        if constexpr(To < num_) {
            get<To>().emplace_back(literal);
            emplace_back_all<To+1>(literal);
        }
    }

public:
    FactorMultiOutput(Outputs&... outputs) : outputs_(std::make_tuple(&outputs...)) {
    }

    FactorMultiOutput(const FactorMultiOutput&) = default;
    FactorMultiOutput(FactorMultiOutput&&) = default;
    FactorMultiOutput& operator=(const FactorMultiOutput&) = default;
    FactorMultiOutput& operator=(FactorMultiOutput&&) = default;

    void emplace_back(const char_t literal) {
        emplace_back_all<>(literal);
    }

    void emplace_back(const index_t src, const index_t len) {
        emplace_back_all<>(src, len);
    }

    void emplace_back(Factor&& f) {
        if(f.is_reference()) {
            emplace_back_all<>(f.src, f.len);
        } else {
            emplace_back_all<>(f.literal());
        }
    }

    void emplace_back(const Factor& f) {
        if(f.is_reference()) {
            emplace_back_all<>(f.src, f.len);
        } else {
            emplace_back_all<>(f.literal());
        }
    }

    template<size_t I>
    const auto& get() const {
        static_assert(I < num_, "out of bounds");
        return *std::get<I>(outputs_);
    }

    template<size_t I>
    auto& get() {
        static_assert(I < num_, "out of bounds");
        return *std::get<I>(outputs_);
    }

    size_t size() const {
        return get<0>().size(); // assumes that size is equal for all
    }
};

}}}
