#pragma once

#include <algorithm>
#include <bit>
#include <cassert>
#include <concepts>
#include <cstdint>
#include <iostream>
#include <limits>
#include <list>
#include <random>
#include <vector>

#include <tdc/io/buffered_reader.hpp>
#include <tdc/math/prime.hpp>
#include <tdc/random/seed.hpp>
#include <tdc/util/char.hpp>
#include <tdc/util/index.hpp>
#include <tdc/util/literals.hpp>

#include "stats.hpp"

namespace tdc {
namespace comp {
namespace lz77 {

template<typename QGram>
class LZSketch {
private:
    static constexpr bool verbose_ = false;
    static constexpr bool verify_ = false;
    static constexpr bool expensive_stats_ = false;

    static constexpr size_t threshold_ = 2;
    static constexpr size_t q_ = sizeof(QGram) / sizeof(char_t);

    struct FilterEntry {
        index_t prev;      // previous occurrence
        index_t count;     // frequency
        index_t old_count; // frequency when entered into filter
    } __attribute__((__packed__));

    class TrieFilter {
    public:
        static constexpr index_t NONE = INDEX_MAX;
        static constexpr index_t ROOT = 0;
    
    private:
        // buckets of minimum data structure
        struct Bucket {
            index_t            count;
            std::list<index_t> leaves;
            std::list<index_t> inner;

            Bucket(const index_t _count) : count(_count) {
            }

            bool empty() const {
                return leaves.empty() && inner.empty();
            }
        };
        
        std::list<Bucket> buckets_;
        using BucketRef = std::list<Bucket>::iterator;
        using MinEntry = std::list<index_t>::iterator;

        BucketRef get_or_create_bucket(BucketRef from, const index_t count) {
            // find successor
            auto it = std::find_if(from, buckets_.end(), [&](const Bucket& bucket){ return bucket.count >= count; });
            if(it == buckets_.end() || it->count != count) {
                // create new bucket
                it = buckets_.emplace(it, count);
            }
            return it;
        }

        // tree structure
        std::vector<char_t>  in_;
        std::vector<index_t> parent_;
        std::vector<index_t> first_child_;
        std::vector<index_t> next_sibling_;
        
        std::vector<index_t> prev_;      // previous occurrence
        std::vector<index_t> count_;     // frequency
        std::vector<index_t> old_count_; // frequency when entered into filter

        std::vector<BucketRef> bucket_;
        std::vector<MinEntry> min_entry_;

        index_t create_node() {
            const auto v = size();
            parent_.emplace_back(NONE);
            in_.emplace_back(0);
            first_child_.emplace_back(NONE);
            next_sibling_.emplace_back(NONE);
            prev_.emplace_back(0);
            count_.emplace_back(0);
            old_count_.emplace_back(0);
            bucket_.emplace_back(buckets_.end());
            min_entry_.emplace_back(std::list<index_t>().end());
            return v;
        }

    public:
        TrieFilter(const size_t initial_capacity = 1) {
            parent_      .reserve(initial_capacity);
            in_          .reserve(initial_capacity);
            first_child_ .reserve(initial_capacity);
            next_sibling_.reserve(initial_capacity);
            
            prev_        .reserve(initial_capacity);
            count_       .reserve(initial_capacity);
            old_count_   .reserve(initial_capacity);

            bucket_      .reserve(initial_capacity);
            min_entry_   .reserve(initial_capacity);

            // insert root
            create_node();
        }

        bool has_min_leaf() const {
            #ifndef NDEBUG
            if(!buckets_.empty()) {
                const bool b = !buckets_.front().leaves.empty();
                //~ if(!b) print_trie(ROOT);
                return b;
            } else {
                return true;
            }
            #else
            return true;
            #endif
        }

        size_t size() const {
            return parent_.size();
        }

        void insert(const index_t v, const index_t parent, const char_t in, const index_t pos, const index_t count, const index_t old_count) {
            parent_[v] = parent;
            in_[v] = in;
            first_child_[v] = NONE;
            
            prev_[v] = pos;
            count_[v] = count;
            old_count_[v] = old_count;

            // make it first child of parent
            const bool parent_was_leaf = is_leaf(parent);
            next_sibling_[v] = first_child_[parent];
            first_child_[parent] = v;

            // insert into minimum data structure
            {
                auto bucket = get_or_create_bucket(buckets_.begin(), count);
                bucket_[v] = bucket;
                
                bucket->leaves.push_front(v);
                min_entry_[v] = bucket->leaves.begin();
            }

            // if parent was a leaf, move it from leaves list to inner node list in its bucket
            if(parent_was_leaf && parent != ROOT) {
                auto parent_bucket = bucket_[parent];
                assert(parent_bucket->count == count_[parent]);  // sanity
                
                auto parent_min_entry = min_entry_[parent];
                assert(*parent_min_entry == parent); // sanity
                
                parent_bucket->leaves.erase(parent_min_entry);
                parent_bucket->inner.push_front(parent);
                min_entry_[parent] = parent_bucket->inner.begin();
            }
            assert(has_min_leaf());
        }

        index_t create_child(const index_t parent, const char_t in, const index_t pos) {
            const auto v = create_node();
            insert(v, parent, in, pos, 1, 0);
            return v;
        }

        index_t get_child_mtf(const index_t node, const char_t c) {
            const auto first_child = first_child_[node];
            auto v = first_child;

            auto prev_sibling = NONE;
            while(v != NONE && in_[v] != c) {
                prev_sibling = v;
                v = next_sibling_[v];
            }

            // move to front
            if(v != NONE && v != first_child) {
                next_sibling_[prev_sibling] = next_sibling_[v];
                next_sibling_[v] = first_child;
                first_child_[node] = v;
            }
            return v;
        }

        index_t count(const index_t node) const {
            return count_[node];
        }

        index_t min_count() const {
            assert(!buckets_.empty());
            return buckets_.front().count;
        }

        index_t extract_min() {
            // select leaf to remove from minimum data structure
            auto bucket = buckets_.begin();
            assert(!bucket->leaves.empty()); // there must be a leaf in the first bucket

            const auto v = bucket->leaves.front();
            assert(count_[v] == bucket->count); // sanity
            assert(is_leaf(v)); // sanity
            bucket->leaves.pop_front();

            // delete empty buckets
            if(bucket->empty()) {
                buckets_.erase(bucket);
            }

            // remove child from parent
            const auto parent = parent_[v];
            {
                index_t prev_sibling = NONE;
                auto x = first_child_[parent];
                while(x != v) {
                    prev_sibling = x;
                    x = next_sibling_[x];
                }
                assert(x == v); // must be child of its parent

                if(prev_sibling != NONE) {
                    next_sibling_[prev_sibling] = next_sibling_[v];
                    assert(!is_leaf(parent));
                } else {
                    first_child_[parent] = next_sibling_[v];
                    if(is_leaf(parent) && parent != ROOT) {
                        // parent has become a leaf
                        auto parent_bucket = bucket_[parent];
                        assert(parent_bucket->count == count_[parent]); // sanity
                        
                        auto parent_min_entry = min_entry_[parent];
                        assert(*parent_min_entry == parent); // sanity
                        
                        parent_bucket->inner.erase(parent_min_entry);
                        parent_bucket->leaves.push_front(parent);
                        min_entry_[parent] = parent_bucket->leaves.begin();
                    }
                }
            }
            assert(has_min_leaf());
            return v;
        }

        index_t prev(const index_t node) const {
            return prev_[node];
        }

        index_t count_delta(const index_t node) const {
            return count_[node] - old_count_[node];
        }

        QGram pattern(const index_t node) const {
            QGram p = in_[node];
            
            auto v = parent_[node];
            while(v != ROOT) {
                p = (p << CHAR_BITS) | in_[v];
                v = parent_[v];
            }
            return p;
        }

        void increment(const index_t node, const index_t pos) {
            // increase key in minimum data structure
            {
                const auto count = count_[node];
                auto bucket = bucket_[node];
                assert(bucket->count == count); // sanity

                auto next_bucket = get_or_create_bucket(bucket, count + 1);
                bucket_[node] = next_bucket;
                
                auto min_entry = min_entry_[node];
                assert(*min_entry == node); // sanity

                if(is_leaf(node)) {
                    bucket->leaves.erase(min_entry);
                    next_bucket->leaves.push_front(node);
                    min_entry_[node] = next_bucket->leaves.begin();
                } else { // inner node
                    bucket->inner.erase(min_entry),
                    next_bucket->inner.push_front(node);
                    min_entry_[node] = next_bucket->inner.begin();
                }

                // delete empty buckets
                if(bucket->empty()) {
                    buckets_.erase(bucket);
                }
            }

            // increment
            ++count_[node];
            prev_[node] = pos;

            assert(has_min_leaf());
        }
    
        bool is_leaf(const index_t node) const {
            return first_child_[node] == NONE;
        }

        size_t num_buckets() const {
            return buckets_.size();
        }

        void print_trie(const index_t node, const size_t indent = 0) const {
            for(size_t i = 0; i < indent; i++) std::cout << " ";
            std::cout << "(" << node << ") ";
            if(node != ROOT) {
                std::cout << "in=" << in_[node];
                std::cout << ", pattern=0x" << std::hex << pattern(node) << std::dec;
                std::cout << ", prev=" << prev_[node];
                std::cout << ", count=" << count_[node];
                std::cout << ", old_count=" << old_count_[node];
            }
            std::cout << std::endl;

            auto v = first_child_[node];
            while(v != NONE) {
                print_trie(v, indent + 4);
                v = next_sibling_[v];
            }
        }

        void print_bucket_histogram() const {
            size_t i = 0;
            for(auto& bucket : buckets_) {
                const auto leaves = bucket.leaves.size();
                const auto inner = bucket.inner.size();
                std::cout << "bucket #" << (++i) << ": count=" << bucket.count << ", leaves=" << leaves << ", inner=" << inner << " -> total=" << (leaves + inner) << std::endl;
            }
        }

        void verify(const index_t node) const {
            #ifndef NDEBUG
            if constexpr(verify_) {
                if(parent_[node] != NONE && parent_[node] != ROOT) {
                    assert(count_[parent_[node]] >= count_[node]);
                }

                auto v = first_child_[node];
                while(v != NONE) {
                    verify(v);
                    v = next_sibling_[v];
                }
            }
            #endif
        }
    };

    class CountMinSketch {
    private:
        index_t                           width_, height_;
        uint64_t                          hash_prime_;
        std::vector<QGram>                hash_mul_;
        std::vector<std::vector<index_t>> count_;

        uint64_t hash(const size_t j, const QGram& pattern) const {
            const uint64_t h = (uint64_t)(pattern * hash_mul_[j]) % hash_prime_;
            if constexpr(verbose_) std::cout << "\t\th_" << j << "(0x" << std::hex << pattern << ")=0x" << h << std::dec << " -> column=" << (h % width_) << std::endl;
            return h % width_;
        }

    public:
        CountMinSketch(size_t width, size_t height) : width_(width), height_(height) {
            hash_prime_ = math::prime_successor(width_);
            
            count_.reserve(height_);
            for(size_t j = 0; j < height_; j++) {
                count_.emplace_back(width, index_t(0));
            }
            
            hash_mul_.reserve(height);

            {
                /*
                 * generate random multipliers such that all nibbles are non-zero
                 */
                static constexpr size_t num_nibbles = (std::numeric_limits<QGram>::digits / 4);
                
                std::default_random_engine gen(random::DEFAULT_SEED);
                std::uniform_int_distribution<QGram> random_nibble(0x1, 0xF); // random
                
                for(size_t j = 0; j < height_; j++) {
                    QGram mul = 0;
                    for(size_t i = 0; i < num_nibbles; i++) {
                        mul = (mul << 4) | random_nibble(gen);
                    }
                    hash_mul_.push_back(mul);
                    if constexpr(verbose_) std::cout << "CM: mul[" << j << "] = 0x" << std::hex << mul << std::dec << std::endl;
                }
            }
        }

        void count(const QGram& pattern, index_t times) {
            for(size_t j = 0; j < height_; j++) {
                count_[j][hash(j, pattern)] += times;
            }
        }

        index_t count_and_estimate(const QGram& pattern, index_t times) {
            index_t est = INDEX_MAX;
            for(size_t j = 0; j < height_; j++) {
                auto& row = count_[j];
                const auto h = hash(j, pattern);
                row[h] += times;
                est = std::min(est, row[h]);
            }
            return est;
        }
    };

    // settings
    size_t max_filter_size_;

    // state
    index_t pos_;
    index_t next_factor_;
    
    QGram   qgram_;

    TrieFilter     trie_;
    CountMinSketch sketch_;

    // stats
    Stats stats_;

    void update_qgram(const char_t c) {
        // update current qgram (first character at lowest significant position)
        static constexpr size_t lsh = CHAR_BITS * (q_ - 1);
        qgram_ = (qgram_ >> CHAR_BITS) | ((QGram)c << lsh);
    }

    void process_qgram(std::ostream& out) {
        // Algorithm 1 and 2, optimized for use of a trie
        if constexpr(verbose_) out << "i=" << pos_ << ", qgram=0x" << std::hex << qgram_ << std::dec << std::endl;
          
        index_t ref_pos = 0;
        index_t ref_len = 0;
        {
            // helpers for quick prefix and character extraction
            auto inv_mask = std::numeric_limits<QGram>::max();
            auto remain = qgram_;

            // begin navigating at trie root
            auto node = TrieFilter::ROOT;
            size_t len = 0;
            while(len < q_) {
                assert(node != TrieFilter::NONE);

                // advance in qgram
                ++len;
                inv_mask <<= CHAR_BITS;
                const auto p = qgram_ & (~inv_mask);
                const auto c = (char_t)remain;
                remain >>= CHAR_BITS;
                if constexpr(verbose_) out << "\tprocessing prefix of len=" << len << ": p=0x" << std::hex << p << std::dec << std::endl;
                
                // try to advance in trie with current character of qgram
                auto v = trie_.get_child_mtf(node, c);
                if(v != TrieFilter::NONE) {
                    // prefix is contained in filter, increment
                    ref_pos = trie_.prev(v);
                    ref_len = len;
                    trie_.increment(v, pos_);
                    ++stats_.num_filter_increments;
                    trie_.verify(TrieFilter::ROOT);
                    if constexpr(verbose_) out << "\t\tincrementing (" << v << ") to " << trie_.count(v) << ", prev=" << ref_pos << std::endl;
                } else if(trie_.size() - 1 < max_filter_size_) {
                    // trie is not yet full - insert new node for prefix
                    v = trie_.create_child(node, c, pos_);
                    ++stats_.num_filter_increments;
                    trie_.verify(TrieFilter::ROOT);
                    if constexpr(verbose_) out << "\t\tinserting edge (" << node << ") -> (" << v << ") with label " << c << std::endl;
                } else {
                    // count prefix in sketch and get estimate
                    const auto est = sketch_.count_and_estimate(p, 1);
                    ++stats_.num_sketch_counts;
                    if constexpr(verbose_) out << "\t\tcounting prefix in sketch: est=" << est << ", trie.min_count=" << trie_.min_count() << std::endl;
                    if(est > trie_.min_count() && est <= trie_.count(node)) {
                        // swap
                        v = trie_.extract_min();
                        trie_.verify(TrieFilter::ROOT);
                        const auto delta = trie_.count_delta(v);
                        const auto pmin = trie_.pattern(v);
                        if constexpr(verbose_) out << "\t\tSWAP with pmin=0x" << std::hex << pmin << std::dec << ", delta=" << delta << std::endl;
                        sketch_.count(pmin, delta);
                        trie_.insert(v, node, c, pos_, est, est);
                        trie_.verify(TrieFilter::ROOT);
                        if constexpr(verbose_) out << "\t\tinserting edge (" << node << ") -> (" << v << ") with label " << c << std::endl;
                        ++stats_.num_swaps;
                    } else {
                        if constexpr(expensive_stats_) {
                            if(est > trie_.min_count()) {
                                if constexpr(verbose_) out << "\t\taborting swap to avoid contradictions" << std::endl;
                                ++stats_.num_contradictions;
                            }
                        }
                        
                        // break out of the loop
                        break;
                    }
                }

                node = v;
            }

            // count remaining prefixes in sketch only
            while(len < q_) {
                // advance in qgram
                ++len;
                inv_mask <<= CHAR_BITS;
                const auto p = qgram_ & (~inv_mask);

                // count prefix in sketch
                if constexpr(verbose_) out << "\tcounting non-frequent prefix in sketch: len=" << len << ": p=0x" << std::hex << p << std::dec << std::endl;
                if constexpr(expensive_stats_) {
                    const auto est = sketch_.count_and_estimate(p, 1);
                    ++stats_.num_sketch_counts;
                    if(est > trie_.min_count()) {
                        if constexpr(verbose_) out << "\t\tprevented contradiction: est=" << est << ", trie.min_count=" << trie_.min_count() << std::endl;
                        ++stats_.num_contradictions;
                    }
                } else {
                    sketch_.count(p, 1);
                    ++stats_.num_sketch_counts;
                }
            }
        }

        if(pos_ >= next_factor_) {
            if(ref_len >= threshold_) {
                output_ref(out, ref_pos, ref_len);
            } else {
                output_literal(out, (char_t)qgram_);
            }
        }
        ++pos_;
    }

    void output_ref(std::ostream& out, const size_t src, const size_t len) {
        ++stats_.num_refs;
        
        // TODO: extensions
        if constexpr(verbose_) out << "-> REFERENCE: (" << src << "," << len << ")" << std::endl;
        else out << "(" << src << "," << len << ")";
        next_factor_ += len;
    }

    void output_literal(std::ostream& out, const char_t c) {
        ++stats_.num_literals;
        
        if constexpr(verbose_) out << "-> LITERAL: " << c << std::endl;
        else out << c;
        ++next_factor_;
    }

public:
    LZSketch(size_t max_filter_size, size_t cm_width, size_t cm_height) : max_filter_size_(max_filter_size), sketch_(cm_width, cm_height) {
    }

    void compress(std::istream& in, std::ostream& out) {
        // init
        size_t read  = 0;
        pos_         = 0;
        next_factor_ = 0;
        qgram_       = 0;

        // read
        {
            io::BufferedReader<char_t> reader(in, 1_Mi);

            // read first q-1 characters
            for(size_t i = 0; i < q_ - 1; i++) {
                update_qgram(reader.read());
                ++read;
            }

            // process stream
            while(reader) {
                update_qgram(reader.read());
                ++read;
                
                process_qgram(out);
            }
        }
        stats_.input_size = read;

        /*
        // process the final qgrams (hash computations may cause collisions!)
        for(size_t i = 0; i < q_- 1; i++) {
            update_qgram(0);
            process_qgram(out);
        }
        */
        for(size_t i = 0; i < q_- 1; i++) {
            update_qgram(0);
            if(pos_ >= next_factor_) {
                output_literal(out, (char_t)qgram_);
            }
            ++pos_;
        }

        // stats
        //~ trie_.print_bucket_histogram();
    }
    
    const Stats& stats() const { return stats_; }
    Stats& stats() { return stats_; }
};

}}}
