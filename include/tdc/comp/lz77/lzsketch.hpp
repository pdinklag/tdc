#pragma once

#include <algorithm>
#include <bit>
#include <cassert>
#include <cmath>
#include <concepts>
#include <cstdint>
#include <iostream>
#include <limits>
#include <list>
#include <random>
#include <vector>

#include <tdc/io/buffered_reader.hpp>
#include <tdc/math/bit_mask.hpp>
#include <tdc/math/ilog2.hpp>
#include <tdc/random/seed.hpp>
#include <tdc/uint/uint_half.hpp>
#include <tdc/util/char.hpp>
#include <tdc/util/index.hpp>
#include <tdc/util/linked_list.hpp>
#include <tdc/util/linked_list_pool.hpp>
#include <tdc/util/literals.hpp>

namespace tdc {
namespace comp {
namespace lz77 {

template<typename QGram>
class LZSketch {
private:
    static constexpr bool verbose_ = false;
    static constexpr bool verify_ = false;
    static constexpr bool expensive_stats_ = false;
    static constexpr bool extended_stats_ = false;

    static constexpr size_t child_array_threshold_ = 24;     // trie nodes with this many children will be converted to child array representation
    static constexpr size_t child_array_low_threshold_ = 16; // trie nodes with less than this many children will be reverted to FCNS representation
    static constexpr size_t child_array_size_ = 1ULL << CHAR_BITS;

    static constexpr size_t threshold_ = 2;
    static constexpr size_t q_ = sizeof(QGram) / sizeof(char_t);
    static constexpr size_t QGRAM_BITS = std::numeric_limits<QGram>::digits;

    class TrieFilter {
    public:
        static constexpr index_t CHILD_ARRAY_MASK = INDEX_MAX >> 1;
        static constexpr index_t CHILD_ARRAY_FLAG = ~CHILD_ARRAY_MASK;
    
        static constexpr index_t NONE = INDEX_MAX >> 1; // nb: must not have CHILD_ARRAY_FLAG set!
        static constexpr index_t ROOT = 0;
    
    private:
        using MinEntry = LinkedListPool<index_t>::Iterator;
    
        // buckets of minimum data structure
        struct Bucket {
            index_t                       count;
            LinkedListPool<index_t>::List nodes;
            index_t                       size; // cached size

            Bucket(LinkedListPool<index_t>& pool, const index_t _count) : count(_count), nodes(pool.new_list()), size(0) {
            }

            void erase(MinEntry e) {
                nodes.erase(e);
                --size;
            }

            MinEntry emplace_front(const index_t v) {
                nodes.emplace_front(v);
                ++size;
                return nodes.begin();
            }

            bool empty() const {
                return size == 0;
            }

            void release() {
                nodes.release();
                nodes = decltype(nodes)();
            }
        };

        LinkedListPool<index_t> node_pool_;
        LinkedList<Bucket> buckets_;

        using BucketRef = LinkedList<Bucket>::Iterator;
        
        // tree structure
        std::vector<char_t>  in_;
        std::vector<index_t> parent_;
        std::vector<index_t> num_children_;
        std::vector<index_t> first_child_;  // when a child array is used (depth < threshold), this has the CHILD_ARRAY_FLAG set and the low bits points to the array
        std::vector<index_t> next_sibling_;

        std::vector<std::array<index_t, child_array_size_>> child_array_;
        std::vector<index_t> free_child_arrays_;
        
        index_t new_child_array() {
            index_t i;
            if(free_child_arrays_.size()) {
                i = free_child_arrays_.back();
                free_child_arrays_.pop_back();
            } else {
                i = child_array_.size();
                child_array_.emplace_back();
            }

            auto& array = child_array_[i];
            for(size_t c = 0; c < child_array_size_; c++) {
                array[c] = NONE;
            }
            return i;
        }

        void free_child_array(const index_t i) {
            free_child_arrays_.emplace_back(i);
        }

        struct Occ {
            index_t prev;
            index_t count;
            BucketRef bucket;
            MinEntry  min_entry;
        };
        
        std::vector<Occ> occ_;
        std::vector<index_t> old_count_; // frequency when entered into filter

        // stats
        size_t max_insert_steps_;
        size_t total_insert_steps_;
        size_t num_inserts_;
        size_t num_child_search_steps_;

        index_t create_node() {
            const auto v = size();
            parent_.emplace_back(NONE);
            in_.emplace_back(0);
            num_children_.emplace_back(0);
            first_child_.emplace_back(NONE);
            next_sibling_.emplace_back(NONE);
            occ_.emplace_back(Occ{0, 0, buckets_.end()});
            old_count_.emplace_back(0);
            return v;
        }

    public:
        TrieFilter(const size_t initial_capacity) : node_pool_(4, initial_capacity), max_insert_steps_(0), total_insert_steps_(0), num_inserts_(0), num_child_search_steps_(0) {
            parent_      .reserve(initial_capacity);
            in_          .reserve(initial_capacity);
            num_children_.reserve(initial_capacity);
            first_child_ .reserve(initial_capacity);
            next_sibling_.reserve(initial_capacity);
            
            occ_         .reserve(initial_capacity);
            old_count_   .reserve(initial_capacity);

            // insert root
            create_node();

            // always give root a child array
            first_child_[ROOT] = CHILD_ARRAY_FLAG | new_child_array();
        }

        size_t size() const {
            return parent_.size();
        }

        void convert_to_child_array(const index_t v) {
            assert(!has_child_array(v));
            
            const auto arr = new_child_array();
            auto child = first_child_[v];
            while(child != NONE) {
                child_array_[arr][in_[child]] = child;
                child = next_sibling_[child];
            }

            first_child_[v] = CHILD_ARRAY_FLAG | arr;
        }

        void revert_from_child_array(const index_t v) {
            assert(has_child_array(v));

            const auto arr = child_array(v);
            
            first_child_[v] = NONE;
            auto prev = NONE;
            for(size_t c = 0; c < child_array_size_; c++) {
                const auto child = child_array_[arr][c];
                if(child != NONE) {
                    if(prev != NONE) {
                        next_sibling_[prev] = child;
                    } else {
                        first_child_[v] = child;
                    }
                    prev = child;
                }
            }
            if(prev != NONE) next_sibling_[prev] = NONE;
            
            free_child_array(arr);
        }

        void insert(const index_t v, const index_t parent, const char_t in, const index_t pos, const index_t count, const index_t old_count) {
            parent_[v] = parent;
            in_[v] = in;
            first_child_[v] = NONE;

            occ_[v].prev = pos;
            occ_[v].count = count;
            old_count_[v] = old_count;

            // make it child of parent
            ++num_children_[parent];
            
            if(has_child_array(parent)) {
                child_array_[child_array(parent)][in] = v;
            } else {
                next_sibling_[v] = first_child_[parent];
                first_child_[parent] = v;

                if(num_children_[parent] >= child_array_threshold_) {
                    // convert parent to using a child array
                    convert_to_child_array(parent);
                }
            }

            // insert into minimum data structure
            {
                if constexpr(expensive_stats_) {
                    size_t steps = 1;
                    auto it = buckets_.begin();
                    while(it != buckets_.end() && it->count < count) {
                        ++it;
                        ++steps;
                    }

                    ++num_inserts_;
                    total_insert_steps_ += steps;
                    max_insert_steps_ = std::max(max_insert_steps_, steps);
                }

                auto bucket = buckets_.begin();
                while(bucket != buckets_.end() && bucket->count < count) ++bucket;
                if(bucket == buckets_.end() || bucket->count != count) {
                    // create new bucket
                    bucket = buckets_.emplace(bucket, node_pool_, count);
                }

                occ_[v].bucket = bucket;
                occ_[v].min_entry = bucket->emplace_front(v);
            }
        }

        index_t create_child(const index_t parent, const char_t in, const index_t pos) {
            const auto v = create_node();
            insert(v, parent, in, pos, 1, 0);
            return v;
        }

        index_t get_child_mtf(const index_t node, const char_t c) {
            if(has_child_array(node)) {
                return child_array_[child_array(node)][c];
            } else {
                const auto first_child = first_child_[node];
                auto v = first_child;

                auto prev_sibling = NONE;
                while(v != NONE && in_[v] != c) {
                    prev_sibling = v;
                    v = next_sibling_[v];
                    if constexpr(expensive_stats_) ++num_child_search_steps_;
                }

                // move to front
                if(v != NONE && v != first_child) {
                    next_sibling_[prev_sibling] = next_sibling_[v];
                    next_sibling_[v] = first_child;
                    first_child_[node] = v;
                }
                return v;
            }
        }

        index_t count(const index_t node) const {
            return occ_[node].count;
        }

        index_t min_count() const {
            assert(!buckets_.empty());
            return buckets_.front().count;
        }

        bool has_child_array(const index_t node) const {
            return first_child_[node] & CHILD_ARRAY_FLAG;
        }

        index_t child_array(const index_t node) const {
            return first_child_[node] & CHILD_ARRAY_MASK;
        }

        index_t extract_min() {
            // select leaf to remove from minimum data structure
            auto bucket = buckets_.begin();
            auto it = bucket->nodes.begin();
            while(!is_leaf(*it)) ++it; // won't run out of bounds, bucket must have a leaf
            
            const auto v = *it;
            assert(occ_[v].count == bucket->count); // sanity
            assert(is_leaf(v)); // sanity
            bucket->erase(it);

            // delete empty buckets
            if(bucket->empty()) {
                bucket->release();
                buckets_.erase(bucket);
            }

            // maybe free child array
            if(has_child_array(v)) {
                free_child_array(child_array(v));
            }

            // remove child from parent
            const auto parent = parent_[v];
            assert(num_children_[parent] > 0);
            --num_children_[parent];
            
            if(has_child_array(parent)) {
                child_array_[child_array(parent)][in_[v]] = NONE;

                if(parent != ROOT && num_children_[parent] < child_array_low_threshold_) {
                    revert_from_child_array(parent);
                }
            } else {
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
                }
            }
            return v;
        }

        index_t prev(const index_t node) const {
            return occ_[node].prev;
        }

        index_t count_delta(const index_t node) const {
            return occ_[node].count - old_count_[node];
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

public:
        void increment(const index_t node, const index_t pos) {
            const auto count = occ_[node].count;

            // increment
            ++occ_[node].count;
            occ_[node].prev = pos;
            
            // increase key in minimum data structure
            {
                auto bucket = occ_[node].bucket;
                assert(bucket->count == count); // sanity

                auto next_bucket = bucket;
                ++next_bucket;

                if(next_bucket == buckets_.end() || next_bucket->count > count+1) {
                    // the next bucket is for > count + 1, we need to create a bucket with exactly count + 1
                    if(bucket->size == 1) {
                        // the current bucket only contains a single element,
                        // we can simply increment the bucket's count by 1
                        ++bucket->count;
                        return;
                    } else {
                        next_bucket = buckets_.emplace(next_bucket, node_pool_, count + 1);
                    }
                }

                occ_[node].bucket = next_bucket;
                
                auto min_entry = occ_[node].min_entry;
                assert(*min_entry == node); // sanity

                bucket->erase(min_entry);
                occ_[node].min_entry = next_bucket->emplace_front(node);

                // delete empty buckets
                if(bucket->empty()) {
                    bucket->release();
                    buckets_.erase(bucket);
                }
            }
        }
    
        bool is_leaf(const index_t node) const {
            return num_children_[node] == 0;
        }

        size_t num_children(const index_t node) const {
            return num_children_[node];
        }

        size_t num_buckets() const {
            return buckets_.size();
        }

        void print_trie(const index_t node = ROOT, const size_t indent = 0) const {
            for(size_t i = 0; i < indent; i++) std::cout << " ";
            std::cout << "(" << node << ") ";
            if(node != ROOT) {
                std::cout << "in=" << in_[node];
                std::cout << ", pattern=0x" << std::hex << pattern(node) << std::dec;
                std::cout << ", children=" << num_children(node);
                std::cout << ", prev=" << occ_[node].prev;
                std::cout << ", count=" << occ_[node].count;
                std::cout << ", old_count=" << old_count_[node];
            }
            std::cout << std::endl;

            if(has_child_array(node)) {
                auto& children = child_array_[child_array(node)];
                for(size_t i = 0; i < child_array_size_; i++) {
                    if(children[i] != NONE) {
                        print_trie(children[i], indent + 4);
                    }
                }
            } else {
                auto v = first_child_[node];
                while(v != NONE) {
                    print_trie(v, indent + 4);
                    v = next_sibling_[v];
                }
            }
        }

        void print_bucket_histogram() const {
            size_t i = 0;
            for(auto& bucket : buckets_) {
                std::cout << "bucket #" << (++i) << ": count=" << bucket.count << ", nodes=" << bucket.size << std::endl;
            }
        }

        void verify(const index_t node) const {
            #ifndef NDEBUG
            if constexpr(verify_) {
                if(parent_[node] != NONE && parent_[node] != ROOT) {
                    assert(occ_[parent_[node]].count >= occ_[node].count);
                }

                auto v = first_child_[node];
                while(v != NONE) {
                    verify(v);
                    v = next_sibling_[v];
                }
            }
            #endif
        }

        template<typename StatLogger>
        void log_stats(StatLogger& logger) {
            if constexpr(expensive_stats_) {
                logger.log("max_insert_steps", max_insert_steps_);
                logger.log("avg_insert_steps", num_inserts_ > 0 ? std::lround((double)total_insert_steps_ / (double)num_inserts_) : 0);
                logger.log("num_child_search_steps", num_child_search_steps_);
            }
        }
    };

    class CountMinSketch {
    private:
        using HalfQGram = uint_half_t<QGram>;

        index_t                           width_mask_;
        index_t                           height_;
        
        std::vector<HalfQGram>            hash_mul_;
        std::vector<std::vector<index_t>> count_;

        index_t hash(const size_t j, const QGram& pattern) const {
            constexpr size_t QGRAM_HALF_BITS = QGRAM_BITS / 2;
            index_t h = ((HalfQGram)pattern * hash_mul_[j]) ^ ((HalfQGram)(pattern >> QGRAM_HALF_BITS) * hash_mul_[j]);

            // modulo 2^19 - 1 (Mersenne prime)
            {
                constexpr index_t MERSENNE19 = (1U << 19) - 1;
                const auto v = h + 1;
                const auto z = ((v >> 19) + v) >> 19;
                h = (h + z) & MERSENNE19;
            }
            
            if constexpr(verbose_) std::cout << "\t\th_" << j << "(0x" << std::hex << pattern << ")=0x" << h << std::dec << " -> column=" << (h & width_mask_) << std::endl;
            return h & width_mask_;
        }

    public:
        CountMinSketch(const size_t width, const size_t height) : height_(height) {
            const auto wbits = math::ilog2_ceil(width - 1);
            width_mask_ = math::bit_mask<index_t>(wbits);
            
            count_.reserve(height_);
            for(size_t j = 0; j < height_; j++) {
                count_.emplace_back(1ULL << wbits, index_t(0));
            }
            
            hash_mul_.reserve(height);

            {
                /*
                 * generate random multipliers such that all nibbles are non-zero
                 */
                static constexpr size_t num_nibbles = (std::numeric_limits<HalfQGram>::digits / 4);
                
                std::default_random_engine gen(random::DEFAULT_SEED);
                std::uniform_int_distribution<HalfQGram> random_nibble(0x1, 0xF); // random
                
                for(size_t j = 0; j < height_; j++) {
                    HalfQGram mul = 0;
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

        template<typename StatLogger>
        void log_stats(StatLogger& logger) {
            logger.log("sketch_width", width_mask_ + 1);
            logger.log("sketch_height", height_);
        }
    };

    // settings
    size_t max_filter_size_;

    // state
    index_t pos_;
    index_t next_factor_;
    
    QGram   qgram_;

    TrieFilter     trie_;
    bool           trie_full_;
    CountMinSketch sketch_;

    // stats
    size_t num_sketch_counts_;
    size_t num_filter_increments_;
    size_t num_swaps_;
    size_t num_contradictions_; // expensive

    void update_qgram(const char_t c) {
        // update current qgram (first character at lowest significant position)
        static constexpr size_t lsh = CHAR_BITS * (q_ - 1);
        qgram_ = (qgram_ >> CHAR_BITS) | ((QGram)c << lsh);
    }

    void filter_increment(const index_t v) {
        trie_.increment(v, pos_);
        ++num_filter_increments_;
        trie_.verify(TrieFilter::ROOT);
        if constexpr(verbose_) std::cout << "\t\tincrementing (" << v << ") to " << trie_.count(v) << ", prev=" << trie_.prev(v) << std::endl;
    }

    index_t sketch_increment(const QGram p, const index_t parent, const char_t c) {
        const auto est = sketch_.count_and_estimate(p, 1);
        ++num_sketch_counts_;
        if constexpr(verbose_) std::cout << "\t\tcounting prefix in sketch: est=" << est << ", trie.min_count=" << trie_.min_count() << std::endl;
        if(est > trie_.min_count() && est <= trie_.count(parent)) {
            // swap
            const auto v = trie_.extract_min();
            trie_.verify(TrieFilter::ROOT);
            const auto delta = trie_.count_delta(v);
            const auto pmin = trie_.pattern(v);
            if constexpr(verbose_) std::cout << "\t\tSWAP with pmin=0x" << std::hex << pmin << std::dec << ", delta=" << delta << std::endl;
            sketch_.count(pmin, delta);
            trie_.insert(v, parent, c, pos_, est, est);
            trie_.verify(TrieFilter::ROOT);
            if constexpr(verbose_) std::cout << "\t\tinserting edge (" << parent << ") -> (" << v << ") with label " << c << std::endl;
            ++num_swaps_;
            return v;
        } else {
            if constexpr(expensive_stats_) {
                if(est > trie_.min_count()) {
                    if constexpr(verbose_) std::cout << "\t\taborting swap to avoid contradictions" << std::endl;
                    ++num_contradictions_;
                }
            }
            return TrieFilter::NONE;
        }
    }

    template<typename FactorOutput>
    void process_qgram(FactorOutput& out) {
        // Algorithm 1 and 2, optimized for use of a trie
        if constexpr(verbose_) std::cout << "i=" << pos_ << ", qgram=0x" << std::hex << qgram_ << std::dec << std::endl;
          
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
                const auto c = (char_t)remain;
                remain >>= CHAR_BITS;
                if constexpr(verbose_) {
                    const auto p = qgram_ & (~inv_mask);
                    std::cout << "\tprocessing prefix of len=" << len << ": p=0x" << std::hex << p << std::dec << std::endl;
                 }
                
                // try to advance in trie with current character of qgram
                auto v = trie_.get_child_mtf(node, c);
                if(v != TrieFilter::NONE) {
                    // prefix is contained in filter, increment
                    ref_pos = trie_.prev(v);
                    ref_len = len;
                    filter_increment(v);
                } else if(!trie_full_) {
                    // trie is not yet full - insert new node for prefix
                    v = trie_.create_child(node, c, pos_);
                    trie_full_ = trie_.size() - 1 == max_filter_size_;
                    
                    ++num_filter_increments_;
                    trie_.verify(TrieFilter::ROOT);
                    if constexpr(verbose_) std::cout << "\t\tinserting edge (" << node << ") -> (" << v << ") with label " << c << std::endl;
                } else {
                    // prefix is infrequent, increment in sketch - may cause a swap!
                    const auto p = qgram_ & (~inv_mask);
                    
                    v = sketch_increment(p, node, c);
                    if(v == TrieFilter::NONE) {
                        // prefix is still infrequent (no swap occurred), break out of the loop
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
                if constexpr(verbose_) std::cout << "\tcounting non-frequent prefix in sketch: len=" << len << ": p=0x" << std::hex << p << std::dec << std::endl;
                if constexpr(expensive_stats_) {
                    const auto est = sketch_.count_and_estimate(p, 1);
                    ++num_sketch_counts_;
                    if(est > trie_.min_count()) {
                        if constexpr(verbose_) std::cout << "\t\tprevented contradiction: est=" << est << ", trie.min_count=" << trie_.min_count() << std::endl;
                        ++num_contradictions_;
                    }
                } else {
                    sketch_.count(p, 1);
                    ++num_sketch_counts_;
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

    template<typename FactorOutput>
    void output_ref(FactorOutput& out, const size_t src, const size_t len) {
        // TODO: extensions
        if constexpr(verbose_) std::cout << "-> REFERENCE: (" << src << "," << len << ")" << std::endl;
        out.emplace_back(src, len);
        next_factor_ += len;
    }

    template<typename FactorOutput>
    void output_literal(FactorOutput& out, const char_t c) {
        if constexpr(verbose_) std::cout << "-> LITERAL: " << c << std::endl;
        out.emplace_back(c);
        ++next_factor_;
    }

public:
    LZSketch(size_t max_filter_size, size_t cm_width, size_t cm_height)
        : max_filter_size_(max_filter_size),
          trie_(max_filter_size_ + 1),
          trie_full_(false),
          sketch_(cm_width, cm_height),
          num_sketch_counts_(0), num_filter_increments_(0), num_swaps_(0), num_contradictions_(0)
    {
    }

    template<typename FactorOutput>
    void compress(std::istream& in, FactorOutput& out) {
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
        if constexpr(extended_stats_) {
            trie_.print_bucket_histogram();
            trie_.print_trie();
        }
    }

    template<typename StatLogger>
    void log_stats(StatLogger& logger) {
        trie_.log_stats(logger);
        sketch_.log_stats(logger);
        
        logger.log("max_filter_size", max_filter_size_);
        logger.log("num_sketch_counts", num_sketch_counts_);
        logger.log("num_filter_increments", num_filter_increments_);
        logger.log("num_swaps", num_swaps_);
        
        if constexpr(expensive_stats_) {
            logger.log("num_contradictions", num_contradictions_);
        }
    }
};

}}}
