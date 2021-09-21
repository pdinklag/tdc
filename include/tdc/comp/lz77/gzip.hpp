#pragma once

#include <cassert>
#include <cstddef>
#include <iostream>

#include <tdc/io/buffered_reader.hpp>
#include <tdc/util/char.hpp>
#include <tdc/util/index.hpp>

#include <tlx/container/ring_buffer.hpp>

namespace tdc {
namespace comp {
namespace lz77 {

class GZip {
private:
    static constexpr bool verbose_ = false;

    static constexpr size_t m1_ = 3;                  // reference threshold
    static constexpr size_t m2_ = 258;                // maximum reference length
    static constexpr size_t dsiz_ = 1ULL << 15;       // window size
    static constexpr size_t dmask_ = dsiz_ - 1;       // mask of lowest 15 bits
    static constexpr size_t hsiz_ = 1ULL << 15;       // size of hash table
    static constexpr size_t hmask_ = hsiz_ - 1;       // mask of lowest 15 bits
    static constexpr size_t d_ = 15ULL / m1_;         // = 3, shift parameter in hashing function

    inline size_t hash(size_t p) {
        size_t h = 0;
        for(size_t i = 0; i < m1_; i++) {
            h = ((h << d_) ^ buf_[p + i]) & hmask_;
        }
        return h;
    }

    tlx::RingBuffer<char_t> buf_;
    index_t buf_offs_; // text position of first entry in buffer
    index_t r_; // current read position in buffer

    index_t pos_; // next text position to encode
    index_t next_factor_;

    index_t* head_;
    index_t* prev_;

    template<typename FactorOutput>
    inline void process(FactorOutput& out) {
        // compute hash
        const size_t h = hash(r_);
        if constexpr(verbose_) std::cout << "\th = 0x" << std::hex << h << std::dec << std::endl;

        // find longest match
        if(pos_ >= next_factor_) {
            size_t longest = 0;
            size_t longest_src = 0;

            size_t k = 0;
            auto src = head_[h];
            while(src + 1 > buf_offs_ && k < dsiz_) { // src >= buf_offs_, also considering -1 being a possible value
                assert(src < pos_);
                
                // match starting at this position
                size_t match = 0;
                size_t i = r_;
                size_t j = src - buf_offs_;
                assert(j < i);
                while(i < buf_.size() && match < m2_ && buf_[i] == buf_[j]) {
                    ++i;
                    ++j;
                    ++match;
                }

                if(match > longest) {
                    longest = match;
                    longest_src = src;
                } else if(match < m1_) {
                    // outdated entry?
                    break;
                }

                // advance in list
                src = prev_[src & dmask_];
                ++k;
            }

            // emit factor
            if(longest >= m1_) {
                if constexpr(verbose_) std::cout << "\t-> (" << longest_src << "," << longest << ")" << std::endl;
                out.emplace_back(longest_src, longest);
                next_factor_ += longest;
            } else {
                if constexpr(verbose_) std::cout << "\t-> 0x" << std::hex << (size_t)buf_[r_] << std::dec << std::endl;
                out.emplace_back(buf_[r_]);
                ++next_factor_;
            }
        }

        // update list
        prev_[pos_ & dmask_] = head_[h];
        head_[h] = pos_;
    }

public:
    inline GZip() : buf_(2 * dsiz_) {
        head_ = new index_t[hsiz_];
        for(size_t i = 0; i < hsiz_; i++) head_[i] = INDEX_MAX;
        
        prev_ = new index_t[dsiz_];
        for(size_t i = 0; i < dsiz_; i++) prev_[i] = INDEX_MAX;
    }

    inline ~GZip() {
        delete[] head_;
        delete[] prev_;
    }

    template<typename FactorOutput>
    inline void compress(std::istream& in, FactorOutput& out) {
        // initialize
        buf_.clear();
        buf_offs_ = 0;
        r_ = 0;
        
        pos_ = 0;
        next_factor_ = 0;
        
        // open file
        io::BufferedReader<char_t> reader(in, dsiz_);

        // read first dsiz_ bytes into  buffer
        while(reader && buf_.size() < dsiz_) {
            buf_.push_back(reader.read());
        }

        // process and read
        while(reader) {
            if constexpr(verbose_) std::cout << "[READING] process pos_=" << pos_ << ", r_=" << r_ << std::endl;
            assert(buf_.size() >= dsiz_);
            process(out);

            if(buf_.size() == buf_.max_size()) {
                // buffer is full
                buf_.pop_front();
                ++buf_offs_;
                assert(r_ == dsiz_);
            } else {
                // still filling up buffer
                ++r_;
            }

            buf_.push_back(reader.read());
            ++pos_;
        }

        // process final window
        while(r_ + m1_ < buf_.size()) {
            //~ if constexpr(verbose_) std::cout << "[REMAIN] process pos_=" << pos_ << ", r_=" << r_ << std::endl;
            process(out);
            
            ++pos_;
            ++r_;
        }

        // emit remaining literals
        while(r_ < buf_.size()) {
            if(pos_ >= next_factor_) {
                //~ if constexpr(verbose_) std::cout << "[REMAIN] output remaining literal 0x" << std::hex << (size_t)buf_[r_] << std::dec << " at pos_=" << pos_ << ", r_=" << r_ << std::endl;
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
