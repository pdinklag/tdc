#pragma once

#include <algorithm>
#include <cassert>
#include <string>
#include <type_traits>
#include <vector>

#include "factor.hpp"

namespace tdc {
namespace comp {
namespace lz77 {

class FactorBuffer {
private:
    class FactorIterator {
    private:
        std::vector<Factor>::const_iterator it_;
        std::vector<Factor>::const_iterator end_;
        size_t pos_;
        Factor current_;

    public:
        FactorIterator(const FactorBuffer& buffer) : it_(buffer.factors_.begin()), end_(buffer.factors_.end()), pos_(0) {
            if(it_ != end_) {
                current_ = *it_++;
            }
        }
        
        operator bool() const {
            return current_.is_valid();
        }
        
        bool operator>(const FactorIterator& other) const {
            return current_.len > other.current_.len || (current_.len == other.current_.len && (current_.src > other.current_.src || current_.is_reference() && other.current_.is_literal()));
        }
        
        bool operator>(const Factor& f) const {
            return current_.len > f.len || (current_.len == f.len && (current_.src > f.src || current_.is_reference() && f.is_literal()));
        }
        
        bool advance_to(const size_t target) {
            while(pos_ < target) {
                if(current_.len <= 1) {
                    // reference of length 1 or literal, advance to next
                    if(it_ != end_) {
                        current_ = *it_++;
                    } else {
                        current_ = Factor();
                        return false;
                    }
                } else {
                    // reference, increment
                    ++current_.src;
                    --current_.len;
                }
                ++pos_;
            }
            return true;
        }
        
        const Factor& operator*() {
            return current_;
        }
    };

public:
    template<typename FactorOutput>
    static void merge(const FactorBuffer& buffer_a, const FactorBuffer& buffer_b, FactorOutput& out) {
        assert(buffer_a.input_size_ == buffer_b.input_size_);
        
        FactorIterator a(buffer_a);
        FactorIterator b(buffer_b);
        
        size_t i = 0;
        size_t num_factors = 0;
        
        while(a && b) {
            // greedily select next factor
            Factor f = a > b ? *a : *b;

            // emit factor
            i += f.decoded_length();
            out.emplace_back(f);
            ++num_factors;

            // advance
            a.advance_to(i);
            b.advance_to(i);
        }

        // done!
        assert(i == buffer_a.input_size_);
        assert(num_factors <= std::min(buffer_a.size(), buffer_b.size()));
    }
    
    template<typename FactorOutput>
    static void merge(const FactorBuffer& buffer_a, const FactorBuffer& buffer_b, const FactorBuffer& buffer_c, FactorOutput& out) {
        assert(buffer_a.input_size_ == buffer_b.input_size_);
        assert(buffer_a.input_size_ == buffer_c.input_size_);
        
        FactorIterator a(buffer_a);
        FactorIterator b(buffer_b);
        FactorIterator c(buffer_c);
        
        size_t i = 0;
        size_t num_factors = 0;
        
        while(a && b && c) {
            // greedily select next factor
            Factor f = *a;
            if(b > f) f = *b;
            if(c > f) f = *c;

            // emit factor
            i += f.decoded_length();
            out.emplace_back(f);
            ++num_factors;

            // advance
            a.advance_to(i);
            b.advance_to(i);
            c.advance_to(i);
        }

        // done!
        assert(i == buffer_a.input_size_);
        assert(num_factors <= std::min(buffer_a.size(), std::min(buffer_b.size(), buffer_c.size())));
    }

private:
    using DecodedString = std::conditional<sizeof(char_t) == 1, std::string,
                            std::conditional<sizeof(char_t) == 2, std::u16string, std::u32string>::type>
                            ::type;

    size_t              input_size_;
    std::vector<Factor> factors_;

public:
    inline FactorBuffer() : input_size_(0) {
    }

    FactorBuffer(const FactorBuffer&) = default;
    FactorBuffer(FactorBuffer&&) = default;
    FactorBuffer& operator=(const FactorBuffer&) = default;
    FactorBuffer& operator=(FactorBuffer&&) = default;

    void emplace_back(const char_t literal) {
        input_size_ += 1;
        factors_.emplace_back(literal);
    }

    void emplace_back(const index_t src, const index_t len) {
        input_size_ += len;
        factors_.emplace_back(src, len);
    }

    void emplace_back(Factor&& f) {
        input_size_ += f.decoded_length();
        factors_.emplace_back(std::move(f));
    }

    void emplace_back(const Factor& f) {
        input_size_ += f.decoded_length();
        factors_.emplace_back(f);
    }

    const Factor* factors() const {
        return factors_.data();
    }

    DecodedString decode() const {
        DecodedString s;
        s.reserve(input_size_);

        for(const auto& f : factors_) {
            if(f.is_reference()) {
                assert(f.src < s.length());
                for(size_t j = 0; j < f.len; j++) {
                    s.push_back(s[f.src + j]);
                }
            } else {
                s.push_back(f.literal());
            }
        }
        assert(s.length() == input_size_);
        return s;
    }

    size_t input_size() const {
        return input_size_;
    }

    size_t size() const {
        return factors_.size();
    }
};

}}}
