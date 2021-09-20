#pragma once

#include <robin_hood.h>

#include <tdc/util/count_min_sketch.hpp>
#include <tdc/util/min_inc.hpp>

namespace tdc {

/// \brief Generic augmented sketch based on hash filters.
///
/// Frequent keys will have an associated value, while non-frequent keys will not.
///
/// \tparam Key key type
/// \tparam Value value type
template<typename Key, typename Value>
class AugmentedSketch {
private:
    struct FilterEntry {
        Value               value;
        index_t             old_count;
        MinInc<Key>::Handle min_handle;
    };

    robin_hood::unordered_map<Key, FilterEntry> filter_;
    MinInc<Key>                                 min_;
    CountMinSketch<Key>                         sketch_;

    size_t max_filter_size_;
    
public:
    AugmentedSketch(size_t max_filter_size, size_t sketch_width, size_t sketch_height) : max_filter_size_(max_filter_size), sketch_(sketch_width, sketch_height) {
    }

    /// \brief Counts the specified key in the sketch once and updates its value.
    /// \param key the key
    /// \param value the value to associate if the key is contained in the filter
    void count(const Key& key, const Value& value) {
        auto it = filter_.find(key);
        if(it != filter_.end()) {
            // frequent item, increase key in minimum data structure and we're done
            const auto new_handle = min_.increase_key(it->second.min_handle);
            it->second = FilterEntry { value, it->second.old_count, new_handle };
        } else {
            // non-frequent
            if(filter_.size() < max_filter_size_) {
                // filter is not yet full, directly insert here with a count of one
                const auto handle = min_.insert(key, 1);
                filter_.emplace(key, FilterEntry { value, 0, handle });
            } else {
                // count key in sketch
                const auto est = sketch_.count_and_estimate(key, 1);
                const auto min_key = min_.min();
                if(est > min_key) {
                    // key is now frequent, swap with minimum item in filter

                    // extract minimum from filter
                    const auto min = min_.extract_min();
                    const auto min_it = filter_.find(min);
                    assert(min_it != filter_.end()); // must exist!
                    assert(min_key >= min_it->second.old_count); // key cannot have decreased
                    const auto delta = min_key - min_it->second.old_count;
                    filter_.erase(min_it);

                    // count erased minimum in sketch
                    sketch_.count(min, delta);

                    // insert newly frequent item into filter
                    const auto handle = min_.insert(key, est);
                    filter_.emplace(key, FilterEntry { value, est, handle });
                }
            }
        }
    }

    /// \brief Tests whether the specified key is frequent.
    ///
    /// If this is the case, the associated value is written to \c out_value.
    ///
    /// \param key the key in question
    /// \param out_value a reference to which to write the associated value
    bool is_frequent(const Key& key, Value& out_value) {
        auto it = filter_.find(key);
        if(it != filter_.end()) {
            out_value = it->second.value;
            return true;
        } else {
            return false;
        }
    }
};

} // namespace tdc
