#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstring>
#include <iostream>

#include <tdc/io/buffered_reader.hpp>
#include <tdc/util/char.hpp>
#include <tdc/util/index.hpp>

namespace tdc {
namespace comp {
namespace lz77 {

class GZip {
private:
    static constexpr size_t m1_ = 3;                  // reference threshold
    static constexpr size_t m2_ = 258;                // maximum reference length
    static constexpr size_t wexp_ = 15;               // window size exponent
    static constexpr size_t dsiz_ = 1ULL << wexp_;    // window size
    static constexpr size_t buf_capacity_ = 2 * dsiz_;
    static constexpr size_t dmask_ = dsiz_ - 1;       // mask of lowest 15 bits
    static constexpr size_t hsiz_ = 1ULL << wexp_;    // size of hash table
    static constexpr size_t hmask_ = hsiz_ - 1;       // mask of lowest 15 bits
    static constexpr size_t d_ = 5;

    inline size_t hash(size_t p) {
        size_t h = 0;
        for(size_t i = 0; i < m1_; i++) {
            h = ((h << d_) ^ buf_[p + i]) & hmask_;
        }
        return h;
    }

    char_t* buf_;
    index_t buf_offs_;  // text position of first entry in buffer
    index_t buf_avail_; // available bytes in buffer
    index_t r_; // current read position in buffer

    index_t pos_; // next text position to encode
    index_t next_factor_;

    index_t* head_;
    index_t* prev_;

    template<typename FactorOutput>
    inline void process(FactorOutput& out) {
        // compute hash
        const size_t h = hash(r_);

        // find longest match
        if(pos_ >= next_factor_) {
            size_t longest = 0;
            size_t longest_src = 0;

            auto src = head_[h];
            while(src + 1 > buf_offs_) { // src >= buf_offs_, also considering -1 being a possible value
                assert(src < pos_);
                
                // match starting at this position
                size_t match = 0;
                size_t i = r_;
                size_t j = src - buf_offs_;
                assert(j < i);
                while(i < buf_avail_ && match < m2_ && buf_[i] == buf_[j]) {
                    ++i;
                    ++j;
                    ++match;
                }

                if(match > longest) {
                    longest = match;
                    longest_src = src;
                } else if(match < m1_) {
                    // collision?
                    break;
                }

                // advance in list
                const auto prev = prev_[src & dmask_];
                if(prev >= src) break;
                src = prev;
            }

            // emit
            if(longest >= m1_) {
                out.emplace_back(longest_src, longest);
            } else {
                longest = std::max(longest, size_t(1));
                for(size_t i = 0; i < longest; i++) {
                    out.emplace_back(buf_[r_ + i]);
                }
            }
            next_factor_ += longest;
        }

        // update list
        prev_[pos_ & dmask_] = head_[h];
        head_[h] = pos_;
    }

public:
    inline GZip() {
        buf_ = new char_t[buf_capacity_];
        
        head_ = new index_t[hsiz_];
        for(size_t i = 0; i < hsiz_; i++) head_[i] = INDEX_MAX;
        
        prev_ = new index_t[dsiz_];
        for(size_t i = 0; i < dsiz_; i++) prev_[i] = INDEX_MAX;
    }

    inline ~GZip() {
        delete[] buf_;
        delete[] head_;
        delete[] prev_;
    }

    template<typename FactorOutput>
    inline void compress(std::istream& in, FactorOutput& out) {
        // initialize
        buf_offs_ = 0;
        buf_avail_ = 0;
        r_ = 0;
        
        pos_ = 0;
        next_factor_ = 0;
        
        // open file
        io::BufferedReader<char_t> reader(in, dsiz_);

        // fill buffer
        buf_avail_ = reader.read(buf_, buf_capacity_);
        while(reader) {
            // process while buffer is full enough
            while(r_ + m2_ < buf_avail_) {
                process(out);
                
                ++r_;
                ++pos_;
            }

            // buffer ran short of maximum reference length, slide
            {
                assert(r_ >= dsiz_);
                
                const size_t ahead = buf_avail_ - r_;
                const size_t discard = r_ - dsiz_;
                const size_t retain = buf_avail_ - discard;

                std::memcpy(buf_, buf_ + discard, retain);

                // read more
                const size_t num_read = reader.read(buf_ + retain, discard);
                //~ std::cout << "Slide: buf_offs_=" << buf_offs_ << ", avail=" << buf_avail_ << ", r=" << r_ << ", ahead=" << ahead << ", discard=" << discard << ", retain=" << retain << ", num_read=" << num_read << std::endl;
                
                buf_avail_ = retain + num_read;
                assert(buf_avail_ <= buf_capacity_);
                buf_offs_ += discard;
                r_ = dsiz_;
            }
        }

        // process final window
        while(r_ + m1_ < buf_avail_) {
            process(out);
            
            ++pos_;
            ++r_;
        }

        // emit remaining literals
        while(r_ < buf_avail_) {
            if(pos_ >= next_factor_) {
                out.emplace_back(buf_[r_]);
                ++next_factor_;
            }

            ++r_;
            ++pos_;
        }
    }

    template<typename StatLogger>
    void log_stats(StatLogger& logger) {
    }
};

}}} // namespace tdc::comp::lz77
