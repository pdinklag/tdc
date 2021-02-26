#pragma once

#include <algorithm>
#include <cassert>
#include <functional>
#include <vector>
#include <utility>

#include "entry.hpp"
#include "linear_probing.hpp"

namespace tdc {
namespace hash {

/// \brief A hash table.
/// \tparam K the key type, must support default construction, copy assignment and equality
template<std::semiregular K, TableEntry<K> E = KeyEntry<K>>
class Table {
public:
    /// \brief The hash function type.
    using hash_func_t  = std::function<size_t(K)>;

    /// \brief The probe function type.
    using probe_func_t = std::function<size_t(size_t)>;

    /// \brief Used to access an entry.
    ///
    /// Note that accessors may become invalid when the underlying table is modified after retrieval.
    class Accessor {
        friend class Table;
        
    private:
        const Table* m_set;
        size_t     m_pos;

        inline Accessor(const Table& table, const size_t pos) : m_set(&table), m_pos(pos) {
        }

    public:
        /// \brief Constructs an invalid accessor.
        inline Accessor() : m_set(nullptr), m_pos(0) {
        }
    
        Accessor(const Accessor&) = default;
        Accessor(Accessor&&) = default;
        Accessor& operator=(const Accessor&) = default;
        Accessor& operator=(Accessor&&) = default;
    
        /// \brief Tells whether the item exists.
        inline bool exists() const {
            return m_set != nullptr;
        }

        /// \brief Shortcut for \ref exists.
        inline operator bool() const {
            return exists();
        }

        /// \brief Retrieves the entry.
        const E& key() const {
            assert(exists());
            return m_set->m_entries[m_pos];
        }

        /// \brief Comparison for standard library use pattern.
        inline bool operator==(const Accessor& other) {
            return m_set == other.m_set && m_pos == other.m_pos;
        }

        /// \brief Comparison for standard library use pattern.
        inline bool operator!=(const Accessor& other) {
            return !(*this == other);
        }
    };

private:
    hash_func_t m_hash_func;
    probe_func_t m_probe_func;
    
    size_t m_cap;
    size_t m_size;
    size_t m_probe_max;
    double m_load_factor;
    double m_growth_factor;

    std::vector<bool> m_used;
    std::vector<E>    m_entries;

    // caches to avoid floating point computations on each insert
    size_t m_size_max;
    size_t m_size_grow;

    // diagnostics
    #ifndef NDEBUG
    size_t m_probe_total;
    size_t m_times_resized;
    #endif

    void init(const size_t capacity) {
        m_size = 0;
        m_cap = capacity;
        m_probe_max = 0;
        #ifndef NDEBUG
        m_probe_total = 0;
        #endif
        
        m_used    = std::vector<bool>(m_cap);
        m_entries = std::vector<E>(m_cap);

        m_size_max = size_t(m_load_factor * (double)m_cap);
        m_size_grow = std::max(m_size_max + 1, size_t((double)m_cap * m_growth_factor));
    }

    size_t hash(const K& key) const {
        return size_t(m_hash_func(key)) % m_cap;
    }

    void insert_internal(const E& entry) {
        const size_t hkey = hash(entry.key());
        
        size_t h = hkey;
        size_t i = 0;
        size_t probe = 0;
        
        while(m_used[h]) {
            i = m_probe_func(i);
            h = (hkey + i) % m_cap;
            ++probe;
            // FIXME: this MAY run infinitely!
        }

        #ifndef NDEBUG
        m_probe_total += probe;
        #endif
        
        m_probe_max = std::max(m_probe_max, probe);
        
        m_used[h] = 1;
        m_entries[h] = entry;
        
        ++m_size;
    }

    void resize(const size_t new_cap) {
        #ifndef NDEBUG
        ++m_times_resized;
        #endif
        
        auto old_cap = m_cap;
        auto used = m_used;
        auto entries = std::move(m_entries);

        init(new_cap);

        for(size_t i = 0; i < old_cap; i++) {
            if(used[i]) insert_internal(entries[i]);
        }
    }

public:
    Table() {
    }

    /// \brief Main constructor.
    /// \param hash_func the hash function to use
    /// \param capacity the initial capacity of the table
    /// \param load_factor the maximum load factor - once reached, the capacity is increased
    /// \param growth_factor the factor for increasing the capacity when the load has been reached
    Table(
        hash_func_t hash_func,
        size_t capacity,
        double load_factor = 1.0,
        double growth_factor = 2.0,
        probe_func_t probe_func = LinearProbing<>{})
        : m_hash_func(hash_func),
          m_load_factor(load_factor),
          m_growth_factor(growth_factor),
          m_probe_func(probe_func) {
              
        #ifndef NDEBUG
        m_times_resized = 0;
        #endif
        
        init(capacity);
    }

    Table(const Table&) = default;
    Table(Table&&) = default;
    Table& operator=(const Table&) = default;
    Table& operator=(Table&&) = default;

    /// \brief Returns the number of items stored in the table.
    inline size_t size() const {
        return m_size;
    }

    /// \brief The current capacity of the table.
    inline size_t capacity() const {
        return m_cap;
    }

    /// \brief The current load of the table.
    inline double load() const {
        return (double)m_size / (double)m_cap;
    }

    /// \brief The maximum number of probe steps performed to resolve a collision.
    inline size_t max_probe() const {
        return m_probe_max;
    }

    #ifndef NDEBUG
    /// \brief The average number of probe steps per contained item.
    inline double avg_probe() const {
        return (double)m_probe_total / (double)m_size;
    }

    /// \brief The number of times the map has grown.
    inline size_t times_resized() const {
        return m_times_resized;
    }
    #endif

    /// \brief Inserts the given entry into the table.
    /// \param entry the entry
    void insert(const E& entry) {
        // first, check if growing is necessary
        if(m_size + 1 > m_size_max) {
            resize(m_size_grow);
        }
        
        // now it's safe to insert
        insert_internal(entry);
    }

    /// \brief Attempts to find the given key and returns an \ref Accessor to the associated item, if any.
    /// \param key the key in question
    Accessor find(const K& key) const {
        const size_t hkey = hash(key);
        
        size_t h = hkey;
        if(m_used[h] && m_entries[h].key() == key) {
            return Accessor(*this, h);
        } else {
            size_t i = 0;
            for(size_t probe = 0; probe < m_probe_max; probe++) {
                i = m_probe_func(i);
                h = (hkey + i) % m_cap;
                if(m_used[h] && m_entries[h].key() == key) {
                    return Accessor(*this, h);
                }
            }
            return Accessor(); // key not found
        }
    }

    /// \brief Returns an invalid accessor.
    ///
    /// This is merely for standard library-style usage.
    Accessor end() const {
        return Accessor();
    }
    
    /// \brief Tests whether a key exists in the table.
    /// \param key the key in question
    bool contains(const K& key) const {
        return find(key) != end();
    }

    /// \brief Erases an item.
    /// \param a the accessor
    bool erase(const Accessor& a) {
        if(a) {
            m_used[a.m_pos] = false;
            --m_size;
            return true;
        } else {
            return false;
        }
    }

    /// \brief Erases an item.
    /// \param key the key of the item
    bool erase(const K& key) {
        return erase(find(key));
    }
};

}} // namespace tdc::hash
