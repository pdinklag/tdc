#pragma once

#include <tdc/util/index.hpp>
#include <tdc/util/linked_list.hpp>
#include <tdc/util/linked_list_pool.hpp>

namespace tdc {

/// \brief Minimum data structure with efficient increase key, based on Space-Saving.
/// \tparam Value value type
template<typename Item>
class MinInc {
private:
    struct Bucket; // fwd
public:
    using Key = index_t;
    using BucketRef = LinkedList<Bucket>::Iterator;
    using MinEntry = LinkedListPool<Item>::Iterator;

    struct Handle {
        BucketRef bucket;
        MinEntry  entry;
    };

private:
    // buckets of minimum data structure
    struct Bucket {
        index_t                     key;
        LinkedListPool<Item>::List  items;
        index_t                     size; // cached size

        Bucket(LinkedListPool<Item>& pool, const index_t _key) : key(_key), items(pool.new_list()), size(0) {
        }

        void erase(MinEntry e) {
            items.erase(e);
            --size;
        }

        MinEntry emplace_front(const Item& item) {
            items.emplace_front(item);
            ++size;
            return items.begin();
        }

        bool empty() const {
            return size == 0;
        }

        void release() {
            items.release();
            items = decltype(items)();
        }
    };

    LinkedListPool<Item> item_pool_;
    LinkedList<Bucket>   buckets_;

public:
    MinInc() {
    }

    /// \brief Inserts a new item with an arbitrary key.
    ///
    /// Note that the running time of this is linear in the key!
    ///
    /// \param item the item to insert
    /// \param key the item's key
    /// \return the item's data structure handle
    Handle insert(const Item& item, const Key& key) {
        // find successor of bucket to insert into
        auto bucket = buckets_.begin();
        while(bucket != buckets_.end() && bucket->key < key) ++bucket;

        // possibly create new bucket for key
        if(bucket == buckets_.end() || bucket->key != key) {
            bucket = buckets_.emplace(bucket, item_pool_, key);
        }
        
        // insert item into bucket return handle
        return Handle { bucket, bucket->emplace_front(item) };
    }

    /// \brief Reports the current minimum key.
    Key min() const {
        assert(!buckets_.empty());
        return buckets_.front().key;
    }

    /// \brief Extracts any item whose key equals the current minimum key.
    Item extract_min() {
        // extract first item from first bucket
        auto bucket = buckets_.begin();
        auto it = bucket->items.begin();

        const auto min_item = *it;
        bucket->erase(it);

        // delete empty buckets
        if(bucket->empty()) {
            bucket->release();
            buckets_.erase(bucket);
        }

        // return
        return min_item;
    }

    /// \brief Increases the key of an item by one.
    /// \param h the item handle
    /// \return the new handle that supersedes the given handle
    Handle increase_key(const Handle& h) {
        // retrieve item
        auto bucket = h.bucket;
        const auto key = bucket->key;

        const auto entry = h.entry;
        const auto item = *entry;

        // retrieve next bucket (into which the item will be moved)
        auto next_bucket = bucket;
        ++next_bucket;

        if(next_bucket == buckets_.end() || next_bucket->key > key+1) {
            // the next bucket is for > key + 1, we need to create a bucket with exactly key + 1
            if(bucket->size == 1) {
                // the current bucket only contains a single element,
                // we can simply increment the bucket's count by 1 and recycle the old handle
                ++bucket->key;
                return h;
            } else {
                next_bucket = buckets_.emplace(next_bucket, item_pool_, key + 1);
            }
        }

        // delete entry from current bucket
        bucket->erase(entry);

        // delete empty buckets
        if(bucket->empty()) {
            bucket->release();
            buckets_.erase(bucket);
        }

        // insert item into next bucket and return handle
        return Handle { next_bucket, next_bucket->emplace_front(item) };
    }
};

} // namespace tdc
