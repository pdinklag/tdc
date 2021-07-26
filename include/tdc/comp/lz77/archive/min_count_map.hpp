#pragma once

#include <cassert>
#include <concepts>
#include <list>
#include <memory>
#include <utility>

#include <robin_hood.h>

#include <tdc/hash/function.hpp>
#include <tdc/hash/table.hpp>
#include <tdc/hash/quadratic_probing.hpp>

#include <tdc/util/index.hpp>

namespace tdc {

/// \brief An simplified variant of \ref MinCount that uses unordered maps for buckets, providing no referential integrity but better cache efficiency.
template<typename key_t>
class MinCountMap {
public:
    struct Entry {
        key_t   key;
        index_t count;
        
        Entry() : key(), count() {
        }
        
        Entry(const key_t& _key, const index_t _count) : key(_key), count(_count) {
        }
        
        Entry(const Entry&) = default;
        Entry(Entry&&) = default;
        Entry& operator=(const Entry&) = default;
        Entry& operator=(Entry&&) = default;
        
        auto operator<=>(const Entry& e) const = default;
        bool operator==(const Entry& e) const = default;
        bool operator!=(const Entry& e) const = default;
    };

private:
    static constexpr uint64_t m_prime = 1069446880303ULL;

    struct Bucket {
        index_t count;
        //robin_hood::unordered_set<key_t> keys;
        hash::Table<key_t> keys;
        Bucket* next;
        Bucket* prev;
        
        Bucket(index_t _count) : count(_count), next(nullptr), prev(nullptr), keys(hash::Modulo(m_prime), 2, 0.75, 1.5) {
        }
        
        bool empty() const {
            return keys.size() == 0;
        }
    };

    Bucket* m_min_bucket;
    robin_hood::unordered_map<size_t, Bucket*> m_buckets;

    void assert_links(Bucket* b) {
#ifndef NDEBUG
        if(b) {
            if(b->next) assert(b->next->prev == b);
            if(b->prev) assert(b->prev->next == b);
        }
#endif
    }

public:
    MinCountMap() : m_min_bucket(nullptr) {
    }
    
    ~MinCountMap() {
        Bucket* b = m_min_bucket;
        while(b) {
            Bucket* next = b->next;
            delete b;
            b = next;
        }
    }

    /// \brief Tests whether the data structure is empty.
    bool empty() const { return m_min_bucket == nullptr; }
    
    /// \brief Gets the number of buckets.
    size_t num_buckets() const { return m_buckets.size(); }

    Entry insert(const key_t key) {
        if(!m_min_bucket || m_min_bucket->count > 1) {
            Bucket* new_min = new Bucket(1);
            new_min->prev = nullptr;
            new_min->next = m_min_bucket;
            if(m_min_bucket) m_min_bucket->prev = new_min;
            m_buckets.emplace(1, new_min);
            m_min_bucket = new_min;
        }
        
        assert_links(m_min_bucket);
        m_min_bucket->keys.insert(key);
        return Entry(key, 1);
    }
    
    void insert(Entry e) {
        Bucket* bucket;
        auto it = m_buckets.find(e.count);
        if(it != m_buckets.end()) {
            // a bucket with the given count already exists
            bucket = it->second;
        } else {
            // find the bucket before which to enter the new bucket
            Bucket* prev = nullptr;
            Bucket* next = m_min_bucket;
            while(next && next->count < e.count) {
                prev = next;
                next = next->next;
            }
            
            bucket = new Bucket(e.count);
            m_buckets.emplace(e.count, bucket);
            
            bucket->prev = prev;
            if(prev) prev->next = bucket;
            else m_min_bucket = bucket;
            
            bucket->next = next;
            if(next) next->prev = bucket;
        }
        
        assert_links(bucket);
        assert_links(bucket->prev);
        assert_links(bucket->next);
        
        bucket->keys.insert(e.key);
    }
    
    Entry insert(const key_t key, const index_t count) {
        auto e = Entry(key, count);
        insert(e);
        return e;
    }
    
    Entry increment(Entry e) {
        auto it = m_buckets.find(e.count);
        if(it != m_buckets.end()) {
            Bucket* bucket = it->second;
            assert_links(bucket);
            if(bucket->keys.erase(e.key)) {
                Bucket* next = bucket->next;
                if(!next || next->count > e.count) {
                    next = new Bucket(e.count + 1);
                    next->prev = bucket;
                    next->next = bucket->next;
                    if(bucket->next) bucket->next->prev = next;
                    bucket->next = next;
                    m_buckets.emplace(e.count + 1, next);
                    assert_links(next);
                }
                
                next->keys.insert(e.key);
                
                if(bucket->empty()) {
                    if(bucket->next) {
                        bucket->next->prev = bucket->prev;
                    }
                    
                    if(bucket->prev) {
                        assert(bucket != m_min_bucket);
                        bucket->prev->next = bucket->next;
                    } else {
                        assert(bucket == m_min_bucket);
                        m_min_bucket = bucket->next;
                    }
                    
                    #ifndef NDEBUG
                    {
                        Bucket* bprev = bucket->prev;
                        Bucket* bnext = bucket->next;
                        
                        bucket->prev = nullptr;
                        bucket->next = nullptr;
                        
                        assert(m_min_bucket != bucket);
                        assert_links(bprev);
                        assert_links(bnext);
                    }
                    #endif
                    
                    m_buckets.erase(e.count);
                    delete bucket;
                }
                
                return Entry(e.key, e.count + 1);
            }
        }
        return e;
    }
    
    Entry increment(const key_t key, const index_t old_count) {
        return increment(Entry(key, old_count));
    }
    
    Entry min() const {
        return Entry(*m_min_bucket->keys.begin(), m_min_bucket->count);
    }
    
    Entry extract_min() {
        auto any_min = min();
        m_min_bucket->keys.erase(any_min.key);
        if(m_min_bucket->empty()) {
            Bucket* old_min = m_min_bucket;
            
            if(old_min->next) {
                m_min_bucket = old_min->next;
                m_min_bucket->prev = nullptr;
            } else {
                m_min_bucket = nullptr;
            }
            
            #ifndef NDEBUG
            old_min->next = nullptr;
            assert_links(m_min_bucket);
            if(m_min_bucket) assert_links(m_min_bucket->next);
            #endif
            
            m_buckets.erase(any_min.count);
            delete old_min;
        }
        return any_min;
    }
};

} // namespace tdc
