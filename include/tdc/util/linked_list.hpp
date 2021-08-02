#pragma once

#include <cassert>
#include <vector>

#include <tdc/util/index.hpp>
#include <tdc/util/likely.hpp>

namespace tdc {

template<typename Item>
class LinkedList {
public:
    class Iterator;

private:
    friend class Iterator;

    static constexpr index_t NONE = INDEX_MAX;

    // linked list management
    index_t head_;
    index_t tail_;

    // items
    struct ItemEntry {
        Item    data;
        index_t prev;
        index_t next;
    };
    
    std::vector<ItemEntry> entry_;
    std::vector<index_t>   free_;

    index_t free_item(Item&& data, const index_t& prev, const index_t& next) {
        index_t item;
        if(free_.size() > 0) {
            item = free_.back();
            free_.pop_back();
            entry_[item] = ItemEntry{std::move(data), prev, next};
        } else {
            item = entry_.size();
            entry_.emplace_back(ItemEntry{std::move(data), prev, next});
        }
        return item;
    }

    void release_item(const index_t item) {
        free_.emplace_back(item);
    }

public:
    class Iterator {
    private:
        friend class LinkedList;

        LinkedList* list_;
        index_t     item_;

        Iterator(LinkedList& list, const index_t item) : list_(&list), item_(item) {
        }

    public:
        Iterator() : list_(nullptr), item_(NONE) {
        }
        
        Iterator(const Iterator&) = default;
        Iterator(Iterator&&) = default;
        Iterator& operator=(const Iterator&) = default;
        Iterator& operator=(Iterator&&) = default;

        bool operator==(const Iterator& other) const {
            return item_ == other.item_;
        }

        bool operator!=(const Iterator& other) const {
            return !(*this == other);
        }

        Item& operator*() {
            return list_->entry_[item_].data;
        }

        Item* operator->() {
            return &(*(*this));
        }

        const Item& operator*() const {
            return list_->entry_[item_].data;
        }

        const Item* operator->() const {
            return &(*(*this));
        }

        Iterator& operator++() {
            item_ = list_->entry_[item_].next;
            return *this;
        }

        Iterator operator++(int) {
            const auto copy = *this;
            ++(*this);
            return copy;
        }
    };

    LinkedList(size_t initial_capacity = 0) : head_(NONE), tail_(NONE) {
        entry_.reserve(initial_capacity);
    }

    template<typename... Args>
    Iterator emplace(const Iterator& it, Args&&... args) {
        const auto next = it.item_;
        const auto prev = next != NONE ? entry_[next].prev : tail_;

        const index_t item = free_item(Item(args...), prev, next);
        
        if(tdc_unlikely(next != NONE)) {
            entry_[next].prev = item;
        } else {
            tail_ = item;
        }

        if(tdc_likely(prev != NONE)) {
            entry_[prev].next = item;
        } else {
            head_ = item;
        }
        verify();
        return Iterator(*this, item);
    }

    void erase(const Iterator& it) {
        const auto item = it.item_;
        const auto prev = entry_[item].prev;
        const auto next = entry_[item].next;

        if(prev != NONE) {
            entry_[prev].next = next;
        } else {
            assert(item == head_);
            head_ = next;
        }

        if(next != NONE) {
            entry_[next].prev = prev;
        } else {
            assert(item == tail_);
            tail_ = prev;
        }

        release_item(item);
        verify();
    }

    Iterator begin() {
        return Iterator(*this, head_);
    }

    Iterator end() {
        return Iterator(*this, NONE);
    }

    const Item& front() const {
        assert(head_ != NONE);
        return entry_[head_].data;
    }

    bool empty() const {
        return head_ == NONE;
    }

    void verify() {
    #ifndef NDEBUG
        auto item = head_;
        auto prev = NONE;
        while(item != NONE) {
            assert(entry_[item].prev == prev);
            prev = item;
            item = entry_[item].next;
        }
        assert(prev == tail_);
    #endif
    }
};

}
