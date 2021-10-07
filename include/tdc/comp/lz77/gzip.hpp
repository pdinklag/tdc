#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstring>
#include <iostream>

#include <tdc/io/buffered_reader.hpp>
#include <tdc/util/index.hpp>

namespace tdc {
namespace comp {
namespace lz77 {

class GZip {
private:
    static constexpr bool track_stats_ = true;

    static constexpr size_t min_match_ = 3;
    static constexpr size_t max_match_ = 258;
    static constexpr size_t window_bits_ = 15;
    static constexpr size_t window_size_ = 1ULL << window_bits_;
    static constexpr size_t buf_capacity_ = 2 * window_size_;
    static constexpr size_t window_mask_ = window_size_ - 1;
    static constexpr size_t num_chains_ = 1ULL << window_bits_;
    static constexpr size_t chain_mask_ = num_chains_ - 1;
    static constexpr size_t hash_shift_ = 5; // if configured to window_bits_ / min_match_, hash function becomes rolling (but isn't used as such)
    static constexpr size_t min_lookahead_ = max_match_ + min_match_ + 1;
    static constexpr size_t max_dist_ = window_size_ - min_lookahead_;
    static constexpr size_t max_chain_length_ = 4096; // gzip - 9
    static constexpr size_t nice_match_ = 258; // gzip - 9
    static constexpr size_t lazy_match_ = 258; // gzip - 9
    static constexpr size_t good_match_ = 32; // gzip - 9
    static constexpr size_t good_laziness_ = 4;
    static constexpr size_t too_far_ = 4096;

    static constexpr index_t NIL = INDEX_MAX;

    inline size_t hash(const size_t p) const {
        size_t h = 0;
        for(size_t i = 0; i < min_match_; i++) {
            h = ((h << hash_shift_) ^ buf_[p + i]) & chain_mask_;
        }
        return h;
    }

    // insert string and return head of hash chain
    inline size_t insert_current_string() {
        const auto h = hash(buf_pos_);
        const auto head = head_[h];
        prev_[pos_ & window_mask_] = head;
        head_[h] = pos_;
        return head;
    }

    uint8_t* buf_;
    index_t buf_offs_;  // text position of first entry in buffer
    index_t buf_avail_; // available bytes in buffer
    index_t buf_pos_; // current read position in buffer

    index_t pos_;         // next text position to encode
    index_t hash_only_;   // number of positions to skip - but still hash - after emitting a reference

    index_t prev_length_;
    index_t prev_src_;
    bool prev_match_exists_;

    index_t match_length_;
    index_t match_src_;

    index_t* head_; // head of hash chains
    index_t* prev_; // chains

    // stats
    size_t stat_chain_length_max_;
    size_t stat_chain_length_total_;
    size_t stat_chain_num_;
    size_t stat_nice_breaks_;
    size_t stat_greedy_skips_;
    size_t stat_good_num_;

    template<typename FactorOutput>
    inline void process(FactorOutput& out) {
        // insert current string
        auto src = insert_current_string();
        if(hash_only_) {
            --hash_only_;
        } else {
            // store previous match
            prev_length_ = match_length_;
            prev_src_ = match_src_;
            match_length_ = min_match_ - 1; // init to horrible

            // find the longest match
            if(src + 1 > buf_offs_ && prev_length_ < lazy_match_ && pos_ - src <= max_dist_) {
                {
                    const uint8_t* buf_end = buf_ + buf_avail_;
                    const size_t max_chain_length = (prev_length_ >= good_match_) ? max_chain_length_ / good_laziness_ : max_chain_length_;

                    if constexpr(track_stats_) {
                        if(prev_length_ >= good_match_) ++stat_good_num_;
                    }

                    size_t chain = 0;
                    match_length_ = prev_length_; // we want to beat the previous match at least

                    const uint16_t scan_start = *(const uint16_t*)(buf_ + buf_pos_);
                    uint8_t scan_end = buf_[buf_pos_ + match_length_];

                    do {
                        // prepare match
                        const uint8_t* p = buf_ + buf_pos_;
                        const uint8_t* q = buf_ + src - buf_offs_;
                        assert(q < p);

                        // if first two characters don't match OR we cannot become better, then don't even bother
                        if(scan_start == *(const uint16_t*)q && scan_end == *(q + match_length_)) {
                            // already matched first two
                            size_t length = 2;
                            p += 2;
                            q += 2;

                            while(p + 1 < buf_end && length < max_match_ && *(const uint16_t*)p == *(const uint16_t*)q) {
                                p += 2;
                                q += 2;
                                length += 2;
                            }

                            while(p < buf_end && length < max_match_ && *p == *q) {
                                ++p;
                                ++q;
                                ++length;
                            }

                            // check match
                            if(length > match_length_) {
                                match_src_ = src;
                                match_length_ = length;

                                if(length >= nice_match_) {
                                    // immediately break when finding a nice match
                                    if constexpr(track_stats_) ++stat_nice_breaks_;
                                    break;
                                }

                                scan_end = buf_[buf_pos_ + match_length_];
                            }
                        }

                        // advance in chain
                        src = prev_[src & window_mask_];
                        ++chain;
                    } while(chain < max_chain_length_ && pos_ - src <= max_dist_);

                    // stats
                    if constexpr(track_stats_) {
                        stat_chain_length_max_ = std::max(stat_chain_length_max_, chain);
                        stat_chain_length_total_ += chain;
                        ++stat_chain_num_;
                    }
                }

                // ignore a minimum match if it is too distant
                if(match_length_ == min_match_ && pos_ - match_src_ > too_far_) {
                    --match_length_;
                }
            }

            // compare current match against previous match
            if(prev_length_ >= min_match_ && match_length_ <= prev_length_) {
                // previous match was better than current, emit
                out.emplace_back(prev_src_, prev_length_);
                hash_only_ = prev_length_ - 2; // current and previous positions are already hashed

                // reset
                match_length_ = min_match_ - 1;
                prev_match_exists_ = false;
            } else if(prev_match_exists_) {
                // current match is better, truncate previous match to a single literal
                assert(buf_pos_ > 0);

                if constexpr(track_stats_) {
                    if(prev_length_ >= min_match_) ++stat_greedy_skips_;
                }

                out.emplace_back(buf_[buf_pos_ - 1]);
            } else {
                // there is no previous match to compare with, wait for next step
                prev_match_exists_ = true;
            }
        }
    }

public:
    inline GZip() {
        buf_ = new uint8_t[buf_capacity_ + min_match_];
        for(size_t i = 0; i < min_match_; i++) buf_[buf_capacity_ + i] = 0;
        
        head_ = new index_t[num_chains_];
        for(size_t i = 0; i < num_chains_; i++) head_[i] = NIL;
        
        prev_ = new index_t[window_size_];
        for(size_t i = 0; i < window_size_; i++) prev_[i] = NIL;
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

        match_src_ = NIL;
        match_length_ = 0;

        prev_src_ = NIL;
        prev_length_ = 0;
        prev_match_exists_ = false;

        hash_only_ = 0;
        
        pos_ = 0;
        
        if constexpr(track_stats_) {
            stat_chain_length_max_ = 0;
            stat_chain_length_total_ = 0;
            stat_chain_num_ = 0;
            stat_nice_breaks_ = 0;
            stat_greedy_skips_ = 0;
            stat_good_num_ = 0;
        }

        // open file
        io::BufferedReader<uint8_t> reader(in, window_size_);

        // fill buffer
        buf_avail_ = reader.read(buf_, buf_capacity_);
        while(reader) {
            // process while buffer has enough bytes left
            assert(buf_avail_ > min_lookahead_);
            {
                const size_t buf_border = buf_avail_ - min_lookahead_;
                while(buf_pos_ < buf_border) {
                    process(out);
                    
                    ++buf_pos_;
                    ++pos_;
                }
            }

            // buffer ran short of min lookahead, slide
            {
                assert(buf_pos_ >= window_size_);

                std::memcpy(buf_, buf_ + window_size_, window_size_);
                buf_pos_ -= window_size_;
                buf_offs_ += window_size_;

                // read more
                const size_t num_read = reader.read(buf_ + window_size_, window_size_);
                buf_avail_ = window_size_ + num_read;
            }
        }

        // process final window
        while(buf_pos_ + min_match_ <= buf_avail_) {
            process(out);
            
            ++pos_;
            ++buf_pos_;
        }

        // emit remaining literals
        while(buf_pos_ < buf_avail_) {
            if(hash_only_) {
                --hash_only_;
            } else {
                out.emplace_back(buf_[buf_pos_]);
            }

            ++buf_pos_;
            ++pos_;
        }
    }

    template<typename StatLogger>
    void log_stats(StatLogger& logger) {
        if constexpr(track_stats_) {
            logger.log("chain_max", stat_chain_length_max_);
            logger.log("chain_avg", (double)stat_chain_length_total_ / (double)stat_chain_num_);
            logger.log("nice_breaks", stat_nice_breaks_);
            logger.log("greedy_skips", stat_greedy_skips_);
            logger.log("good_num", stat_good_num_);
        }
    }
};

}}} // namespace tdc::comp::lz77
