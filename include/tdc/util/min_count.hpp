#pragma once

#include <cassert>
#include <memory>
#include <utility>

#include <tdc/util/index.hpp>

namespace tdc {

/// \brief The data structure of Metwally et al., 2005, to maintain a set of items under constant-time minimum extraction and count increment operations.
template<typename item_t>
class MinCount {
public:
    class Entry; // fwd

private:
    class Bucket {
    private:
        index_t m_count;
        
    public:
        std::unique_ptr<Bucket> m_next;
        Bucket* m_prev;
        std::unique_ptr<Entry> m_head;

        Bucket(const index_t count) : m_count(count), m_prev(nullptr) {
        }
        
        index_t count() const { return m_count; }
        bool empty() const { return !m_head; }
        
        Entry& insert(std::unique_ptr<Entry>&& e) {
            assert(!e->m_bucket);
            assert(!e->m_prev);
            assert(!e->m_next);
            
            if(m_head) m_head->m_prev = e.get();
            e->m_next = std::move(m_head);
            e->m_bucket = this;
            m_head = std::move(e);
            return *m_head;
        }
        
        Entry& insert(item_t&& item) {
            return insert(std::make_unique<Entry>(std::move(item)));
        }
        
        std::unique_ptr<Entry> extract_head() {
            if(m_head) {
                m_head->m_bucket = nullptr;
                
                auto ret = std::move(m_head);
                m_head = std::move(ret->m_next);
                if(m_head) m_head->m_prev = nullptr;
                return ret;
            } else {
                return std::unique_ptr<Entry>();
            }
        }
        
        std::unique_ptr<Entry> remove(Entry& e) {
            assert(e.m_bucket == this);
            if(&e == m_head.get()) {
                // removing head
                return extract_head();
            } else {
                // removing from chain
                assert(e.m_prev);
                assert(e.m_prev->m_next.get() == &e);
                
                auto ret = std::move(e.m_prev->m_next);
                if(ret->m_next) ret->m_next->m_prev = ret->m_prev;
                ret->m_prev->m_next = std::move(ret->m_next);
                ret->m_bucket = nullptr;
                ret->m_prev = nullptr;
                return ret;
            }
        }
    };
    
public:
    class Entry {
    private:
        friend class Bucket;
        friend class MinCount;
    
        Bucket* m_bucket;
        Entry* m_prev;
        std::unique_ptr<Entry> m_next;
        
        item_t m_item;

    public:
        Entry(item_t&& item) : m_bucket(nullptr), m_prev(nullptr), m_item(std::move(item)) {
        }
        
        index_t count() const { return m_bucket ? m_bucket->count() : 0; }
        const item_t& item() const { return m_item; }
    };

private:
    std::unique_ptr<Bucket> m_first;

    Bucket* create_bucket(Bucket* after, index_t count) {
        // create new bucket for count
        auto new_bucket = std::make_unique<Bucket>(count);
        Bucket* b = new_bucket.get();
        
        // insert after previous if it exists, otherwise make it the first bucket
        if(after) {
            new_bucket->m_next = std::move(after->m_next);
            new_bucket->m_prev = after;
            after->m_next = std::move(new_bucket);
        } else {
            m_first = std::move(new_bucket);
        }
        
        return b;
    }

    Bucket* remove_bucket(Bucket& bucket) { // returns previous bucket, if any
        if(&bucket == m_first.get()) {
            // removing first bucket
            m_first = std::move(bucket.m_next);
            return nullptr;
        } else {
            assert(bucket.m_prev);
            Bucket* prev = bucket.m_prev;
            
            // removing bucket from chain
            if(bucket.m_next) bucket.m_next->m_prev = prev;
            prev->m_next = std::move(bucket.m_next);
            return prev;
        }
    }

public:
    MinCount() {
    }

    /// \brief Extracts the count of the entries with the minimum count.
    index_t min() const {
        return m_first ? m_first->count() : 0;
    }

    /// \brief Extracts an entry with the minimum count.
    std::pair<std::unique_ptr<Entry>, index_t> extract_min() {
        if(m_first) {
            auto min = m_first->extract_head();
            const index_t count = m_first->count();
            if(m_first->empty()) {
                // make next bucket the new first
                m_first = std::move(m_first->m_next);
            }
            return std::pair(std::move(min), count);
        } else {
            return std::pair(std::unique_ptr<Entry>(), 0);
        }
    }

    /// \brief Inserts a new item with count one.
    /// \param item the item
    Entry& insert(item_t&& item) {
        // find bucket
        Bucket* b;
        if(m_first && m_first->count() == 1) {
            b = m_first.get();
        } else {
            b = create_bucket(nullptr, 1);
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
        
        // first, remove item from current bucket
        Bucket* b = e.m_bucket;
        
        auto e_owning_ptr = b->remove(e);
        if(b->empty()) {
            // remove empty bucket
            b = remove_bucket(*b);
        }
        
        // now, insert it into the next bucket or create it
        {
            Bucket* next = b ? b->m_next.get() : nullptr;
            if(next && next->count() == new_count) {
                b = next;
            } else {
                b = create_bucket(b, new_count);
            }
        }
        
        b->insert(std::move(e_owning_ptr));
    }
};

} // namespace tdc
