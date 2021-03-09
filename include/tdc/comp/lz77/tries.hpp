#pragma once

#include <concepts>
#include <cstdint>
#include <utility>
#include <vector>

#include <robin_hood.h>

#include <tdc/util/index.hpp>

namespace tdc {
namespace comp {
namespace lz77 {

template<std::unsigned_integral char_t>
class TrieHash {
private:
    using unique_ptr = std::unique_ptr<TrieHash>;
    robin_hood::unordered_map<char_t, unique_ptr> m_children;
    
    index_t m_count;
    index_t m_last_seen_at;
    
public:
    TrieHash() : m_count(0), m_last_seen_at(0) {
    }
    
    TrieHash* get_or_create_child(const char_t c) {
        auto it = m_children.find(c);
        if(it != m_children.end()) {
            return it->second.get();
        } else {
            auto* child = new TrieHash();
            m_children.emplace(c, unique_ptr(child));
            return child;
        }
    }
    
    size_t count() const { return m_count; };
    size_t last_seen_at() const { return m_last_seen_at; };
    
    void update(const size_t count, const size_t last_seen_at) {
        m_count = count;
        m_last_seen_at = last_seen_at;
    }
};

template<std::unsigned_integral char_t>
class TrieList {
private:
    using unique_ptr = std::unique_ptr<TrieList>;
    
    struct Child {
        unique_ptr p;
        char_t     c;
    };
      
    std::vector<Child> m_children;
    
    index_t m_count;
    index_t m_last_seen_at;
    
public:
    TrieList() : m_count(0), m_last_seen_at(0) {
    }
    
    TrieList* get_or_create_child(const char_t c) {
        for(auto& x : m_children) {
            if(x.c == c) return x.p.get();
        }
        
        auto* child = new TrieList();
        m_children.push_back(Child{unique_ptr(child), c});
        return child;
    }
    
    size_t count() const { return m_count; };
    size_t last_seen_at() const { return m_last_seen_at; };
    
    void update(const size_t count, const size_t last_seen_at) {
        m_count = count;
        m_last_seen_at = last_seen_at;
    }
};

}}} // namespace tdc::comp::lz77
