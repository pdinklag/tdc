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
    static constexpr size_t min_match_ = 3;
    static constexpr size_t max_match_ = 258;
    static constexpr size_t window_bits_ = 15;
    static constexpr size_t window_size_ = 1ULL << window_bits_;
    static constexpr size_t buf_capacity_ = 2 * window_size_;
    static constexpr size_t window_mask_ = window_size_ - 1;
    static constexpr size_t num_chains_ = 1ULL << window_bits_;
    static constexpr size_t chain_mask_ = num_chains_ - 1;
    static constexpr size_t hash_shift_ = 5; // if configured to window_bits_ / min_match_, hash function becomes rolling (but isn't used as such)
    static constexpr size_t max_chain_length_ = 4096;
    static constexpr size_t nice_match_ = 258;

    inline size_t hash(size_t p) {
        size_t h = 0;
        for(size_t i = 0; i < min_match_; i++) {
            h = ((h << hash_shift_) ^ buf_[p + i]) & chain_mask_;
        }
        return h;
    }

    char_t* buf_;
    index_t buf_offs_;  // text position of first entry in buffer
    index_t buf_avail_; // available bytes in buffer
    index_t buf_pos_; // current read position in buffer

    index_t pos_;         // next text position to encode
    index_t next_factor_; // next text position at which we will encode something

    index_t* head_; // head of hash chains
    index_t* prev_; // chains

    template<typename FactorOutput>
    inline void process(FactorOutput& out) {
        // compute hash
        const size_t h = hash(buf_pos_);

        // find longest match
        if(pos_ >= next_factor_) {
            size_t longest = 0;
            size_t longest_src = 0;

            auto src = head_[h];
            auto chain = max_chain_length_;
            while(chain && src + 1 > buf_offs_) { // src >= buf_offs_, also considering -1 being a possible value
                assert(src < pos_);
                
                // match starting at this position
                size_t match = 0;
                size_t i = buf_pos_;
                size_t j = src - buf_offs_;
                assert(j < i);
                while(i < buf_avail_ && match < max_match_ && buf_[i] == buf_[j]) {
                    ++i;
                    ++j;
                    ++match;
                }

                if(match > longest) {
                    longest = match;
                    longest_src = src;

                    if(match >= nice_match_) {
                        break; // immediately stop when hitting a nice match
                    }
                }

                // advance in list
                const auto prev = prev_[src & window_mask_];
                if(prev >= src) break;
                src = prev;
                --chain;
            }

            // emit
            if(longest >= min_match_) {
                out.emplace_back(longest_src, longest);
            } else {
                longest = std::max(longest, size_t(1));
                for(size_t i = 0; i < longest; i++) {
                    out.emplace_back(buf_[buf_pos_ + i]);
                }
            }
            next_factor_ += longest;
        }

        // update list
        prev_[pos_ & window_mask_] = head_[h];
        head_[h] = pos_;
    }

public:
    inline GZip() {
        buf_ = new char_t[buf_capacity_];
        
        head_ = new index_t[num_chains_];
        for(size_t i = 0; i < num_chains_; i++) head_[i] = INDEX_MAX;
        
        prev_ = new index_t[window_size_];
        for(size_t i = 0; i < window_size_; i++) prev_[i] = INDEX_MAX;
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
        buf_pos_ = 0;
        
        pos_ = 0;
        next_factor_ = 0;
        
        // open file
        io::BufferedReader<char_t> reader(in, window_size_);

        // fill buffer
        buf_avail_ = reader.read(buf_, buf_capacity_);
        while(reader) {
            // process while buffer is full enough
            while(buf_pos_ + max_match_ < buf_avail_) {
                process(out);
                
                ++buf_pos_;
                ++pos_;
            }

            // buffer ran short of maximum reference length, slide
            {
                assert(buf_pos_ >= window_size_);
                
                const size_t ahead = buf_avail_ - buf_pos_;
                const size_t discard = buf_pos_ - window_size_;
                const size_t retain = buf_avail_ - discard;

                std::memcpy(buf_, buf_ + discard, retain);

                // read more
                const size_t num_read = reader.read(buf_ + retain, discard);
                //~ std::cout << "Slide: buf_offs_=" << buf_offs_ << ", avail=" << buf_avail_ << ", r=" << r_ << ", ahead=" << ahead << ", discard=" << discard << ", retain=" << retain << ", num_read=" << num_read << std::endl;
                
                buf_avail_ = retain + num_read;
                assert(buf_avail_ <= buf_capacity_);
                buf_offs_ += discard;
                buf_pos_ = window_size_;
            }
        }

        // process final window
        while(buf_pos_ + min_match_ < buf_avail_) {
            process(out);
            
            ++pos_;
            ++buf_pos_;
        }

        // emit remaining literals
        while(buf_pos_ < buf_avail_) {
            if(pos_ >= next_factor_) {
                out.emplace_back(buf_[buf_pos_]);
                ++next_factor_;
            }

            ++buf_pos_;
            ++pos_;
        }
    }

    template<typename StatLogger>
    void log_stats(StatLogger& logger) {
    }
};

}}} // namespace tdc::comp::lz77
