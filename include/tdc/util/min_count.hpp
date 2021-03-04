#pragma once

#include <cassert>
#include <list>
#include <memory>
#include <utility>

#include <tdc/util/index.hpp>

namespace tdc {

/// \brief The data structure of Metwally et al., 2005, to maintain a set of items under constant-time minimum extraction and count increment operations.
template<typename item_t, bool m_delete_empty = true>
class MinCount {
private:
    class Bucket;
    using BucketRef = std::list<Bucket>::iterator;
    using ItemRef = std::list<item_t>::iterator;
    
public:
    struct Entry {
    private:
        friend class Bucket;
        friend class MinCount;
    
        Bucket* m_bucket;
        ItemRef m_it;
        
        Entry(Bucket* bucket, decltype(m_it) it) : m_bucket(bucket), m_it(it) {
        }
        
    public:
        Entry() : m_bucket(nullptr) {
        }
    
        bool valid() const { return m_bucket != nullptr; }
        index_t count() const { return m_bucket->count(); }
        item_t& item() { return *m_it; }
    };

private:
    class Bucket {
    private:
        index_t m_count;
        std::list<item_t> m_entries;
        BucketRef m_it;
    
    public:
        Bucket(const index_t count) : m_count(count) {
        }
        
        index_t count() const { return m_count; }
        bool empty() const { return m_entries.empty(); }
        
        auto it() { return m_it; }
        void it(decltype(m_it) _it) { m_it = _it; }
        
        Entry insert(item_t&& item) {
            m_entries.emplace_front(std::move(item));
            return Entry(this, m_entries.begin());
        }
        
        item_t extract_head() {
            assert(!empty());
            
            auto item = m_entries.front();
            m_entries.pop_front();
            return item;
        }
        
        item_t remove(Entry& e) {
            assert(e.m_bucket == this);
            
            auto item = *e.m_it;
            m_entries.erase(e.m_it);
            e = Entry();
            return item;
        }
    };

private:
    std::list<Bucket> m_buckets;
    BucketRef m_min;

    BucketRef remove_bucket(BucketRef bucket) {
        BucketRef next = bucket;
        ++next;
        m_buckets.remove(bucket);
    }

public:
    MinCount() {
        m_min = m_buckets.end();
    }
    
    /// \brief Tests whether the data structure is empty.
    bool empty() const { return m_min == m_buckets.end(); }

    /// \brief Extracts the count of the entries with the minimum count.
    index_t min() const {
        assert(!empty());
        return m_min->count();
    }

    /// \brief Extracts an entry with the minimum count.
    item_t extract_min() {
        assert(!empty());
        auto& bucket = *m_min;
        auto item = bucket.extract_head();
        
        if(bucket.empty()) {
            if constexpr(m_delete_empty) {
                m_buckets.pop_front();
                m_min = m_buckets.begin();
            } else {
                do { ++m_min; } while(m_min != m_buckets.end() && m_min->empty());
            }
        }
        
        return item;
    }
    
    /// \brief Inserts a new item.
    /// \param item the item
    /// \param count the initial count
    Entry insert(item_t&& item, const index_t count = 1) {
        // find bucket
        auto b = m_buckets.begin();
        while(b != m_buckets.end() && b->count() < count) {
            ++b;
        }
        
        // create bucket if it doesn't yet exist
        if(b == m_buckets.end() || b->count() > count) {
            b = m_buckets.emplace(b, Bucket(count));
            b->it(b);
            
            if(m_min == m_buckets.end() || count < m_min->count()) {
                m_min = b;
            }
        }
        
        // insert item
        return b->insert(std::move(item));
    }

    /// \brief Increments the count of the given entry.
    /// \param the entry in question
    void increment(Entry& e) {
        assert(e.m_bucket);
        
        // increment count
        const index_t new_count = e.count() + 1;
        
        // remove item from current bucket
        auto& old_bucket = *e.m_bucket;
        auto item = old_bucket.remove(e);
        
        // maybe remove bucket if empty
        auto b = old_bucket.it();
        if(old_bucket.empty()) {
            if constexpr(m_delete_empty) {
                if(b == m_min) ++m_min;
                b = m_buckets.erase(b);
            } else {
                if(b == m_min) {
                    do { ++m_min; } while(m_min != m_buckets.end() && m_min->empty());
                }
            }
        } else {
            ++b;
        }
        
        // maybe create new bucket
        if(b == m_buckets.end() || b->count() > new_count) {
            b = m_buckets.emplace(b, Bucket(new_count));
            b->it(b);
        }
        
        // insert item
        e = b->insert(std::move(item));
    }
};

} // namespace tdc
