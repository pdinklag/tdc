#pragma once

#include <cassert>
#include <concepts>
#include <list>
#include <memory>
#include <utility>

#include <robin_hood.h>

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
    struct Bucket {
        index_t count;
        robin_hood::unordered_set<key_t> keys;
        Bucket* next;
        Bucket* prev;
        
        Bucket(index_t _count) : count(_count), next(nullptr), prev(nullptr) {
        }
        
        bool empty() const {
            return keys.size() == 0;
        }
    };

    Bucket* m_min_bucket;
    robin_hood::unordered_map<size_t, Bucket*> m_buckets;

public:
    MinCountMap() : m_min_bucket(nullptr) {
    }

    /// \brief Tests whether the data structure is empty.
    bool empty() const { return m_min_bucket == nullptr; }
    
    /// \brief Gets the number of buckets.
    size_t num_buckets() const { return m_buckets.size(); }

    Entry insert(key_t key) {
        if(!m_min_bucket || m_min_bucket->count > 1) {
            Bucket* new_min = new Bucket(1);
            new_min->prev = nullptr;
            new_min->next = m_min_bucket;
            m_buckets.emplace(1, new_min);
            m_min_bucket = new_min;
        }
        
        m_min_bucket->keys.insert(key);
        return Entry(key, 1);
    }
    
    Entry increment(Entry e) {
        auto it = m_buckets.find(e.count);
        if(it != m_buckets.end()) {
            Bucket* bucket = it->second;
            if(bucket->keys.erase(e.key)) {
                Bucket* next = bucket->next;
                if(!next || next->count > e.count) {
                    next = new Bucket(e.count + 1);
                    next->prev = bucket;
                    next->next = bucket->next;
                    bucket->next = next;
                    m_buckets.emplace(e.count + 1, next);
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
                    
                    assert(m_min_bucket != bucket);
                    if(bucket->next) assert(bucket->next->prev != bucket);
                    if(bucket->prev) assert(bucket->prev->next != bucket);
                    
                    m_buckets.erase(e.count);
                    delete bucket;
                }
                
                return Entry(e.key, e.count + 1);
            }
        }
        return e;
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
                old_min->next->prev = nullptr;
                m_min_bucket = old_min->next;
            } else {
                m_min_bucket = nullptr;
            }
            
            m_buckets.erase(any_min.count);
            delete old_min;
        }
        return any_min;
    }
};

} // namespace tdc
