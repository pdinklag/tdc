#pragma once

#include <concepts>

namespace tdc {
namespace hash {

template<typename E, typename K>
concept TableEntry =
    requires(const E& entry) {
        { entry.key() } -> std::same_as<K>;
    };

template<typename K>
struct KeyEntry {
    K m_key;
    
    KeyEntry() {
    }
    
    KeyEntry(K key) : m_key(key) {
    }

    K key() const {
        return m_key;
    }
    
    operator K() const {
        return m_key;
    }
};

template<typename K, typename V>
struct KeyValueEntry {
    K m_key;
    V m_value;
    
    KeyValueEntry() {
    }
    
    KeyValueEntry(K key, V value) : m_key(key), m_value(value) {
    }

    K key() const {
        return m_key;
    }
    
    V value() const {
        return m_value;
    }
};

}} // namespace tdc::hash
