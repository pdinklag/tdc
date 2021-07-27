#pragma once

#include <cassert>
#include <vector>

#include <tdc/util/index.hpp>

namespace tdc {

template<typename Item>
class LinkedListPool {
public:
    class List;
    class Iterator;

private:
    friend class List;
    friend class Iterator;

    static constexpr index_t NONE = INDEX_MAX;

    // linked list management
    std::vector<index_t> list_head_;
    std::vector<index_t> list_free_;

    index_t free_list() {
        index_t list;
        if(list_free_.size() > 0) {
            // recycle
            list = list_free_.back();
            list_free_.pop_back();
            list_head_[list] = NONE;
        } else {
            // allocate new
            list = list_head_.size();
            list_head_.emplace_back(NONE);
        }
        return list;
    }

    void release_list(const index_t list) {
        auto item = list_head_[list];
        while(item != NONE) {
            auto next = item_entry_[item].next;
            release_item(item);
            item = next;
        }
        list_free_.emplace_back(list);
    }

    // items
    struct ItemEntry {
        Item    data;
        index_t prev;
        index_t next;
    };
    
    std::vector<ItemEntry> item_entry_;
    std::vector<index_t>   item_free_;

    index_t free_item(Item&& data, const index_t& prev, const index_t& next) {
        index_t item;
        if(item_free_.size() > 0) {
            item = item_free_.back();
            item_free_.pop_back();
            item_entry_[item] = ItemEntry{std::move(data), prev, next};
        } else {
            item = item_entry_.size();
            item_entry_.emplace_back(ItemEntry{std::move(data), prev, next});
        }
        return item;
    }

    void release_item(const index_t item) {
        item_free_.emplace_back(item);
    }

public:
    class List {
    private:
        friend class LinkedListPool;
    
        LinkedListPool* pool_;
        index_t         list_;

        List(LinkedListPool& pool) : pool_(&pool) {
            list_ = pool_->free_list();
        }

    public:
        List() : pool_(nullptr), list_(NONE) {
        }

        ~List() {
            if(pool_) pool_->release_list(list_);
        }
    
        List(const List&) = delete;
        List& operator=(const List&) = delete;
        
        List(List&& other) {
            *this = std::move(other);
        }

        List& operator=(List&& other) {
            pool_ = other.pool_;
            list_ = other.list_;

            // prevent other from being released
            other.pool_ = nullptr;
            other.list_ = NONE;
        }

        template<typename... Args>
        void emplace_front(Args&&... args) {
            auto& head = pool_->list_head_[list_];
            const index_t item = pool_->free_item(Item(args...), NONE, head);

            if(head != NONE) {
                pool_->item_entry_[head].prev = item;
            }
            
            head = item;
        }

        Iterator begin() {
            return Iterator(*pool_, pool_->list_head_[list_]);
        }

        Iterator end() {
            return Iterator(*pool_, NONE);
        }

        void erase(const Iterator& it) {
            assert(size() > 0);
            const index_t item = it.item_;
            const index_t prev = pool_->item_entry_[item].prev;
            const index_t next = pool_->item_entry_[item].next;

            if(next != NONE) {
                pool_->item_entry_[next].prev = prev;
            }
            
            if(prev != NONE) {
                pool_->item_entry_[prev].next = next;
            } else {
                auto& head = pool_->list_head_[list_];
                assert(item == head);
                head = next;
            }
            
            pool_->release_item(item);
        }

        void verify() {
        #ifndef NDEBUG
            size_t count = 0;
            auto item = pool_->list_head_[list_];
            auto prev = NONE;
            while(item != NONE) {
                ++count;
                assert(pool_->item_entry_[item].prev == prev);
                prev = item;
                item = pool_->item_entry_[item].next;
            }
            assert(count == size());
        #endif
        }
    };

    class Iterator {
    private:
        friend class List;

        LinkedListPool* pool_;
        index_t         item_;

        Iterator(LinkedListPool& pool, const index_t item) : pool_(&pool), item_(item) {
        }

    public:
        Iterator() : pool_(nullptr), item_(NONE) {
        }
        
        Iterator(const Iterator&) = default;
        Iterator(Iterator&&) = default;
        Iterator& operator=(const Iterator&) = default;
        Iterator& operator=(Iterator&&) = default;

        bool operator==(const Iterator& other) const {
            return pool_ == other.pool_ && item_ == other.item_;
        }

        bool operator!=(const Iterator& other) const {
            return !(*this == other);
        }

        Item& operator*() {
            return pool_->item_entry_[item_].data;
        }

        Item* operator->() {
            return &(*(*this));
        }

        const Item& operator*() const {
            return pool_->item_entry_[item_].data;
        }

        const Item* operator->() const {
            return &(*(*this));
        }

        Iterator& operator++() {
            item_ = pool_->item_entry_[item_].next;
            return *this;
        }

        Iterator operator++(int) {
            const auto copy = *this;
            ++(*this);
            return copy;
        }
    };

    LinkedListPool(size_t initial_list_capacity = 0, size_t initial_item_capacity = 0) {
        list_head_.reserve(initial_list_capacity);
        item_entry_.reserve(initial_item_capacity);
    }

    List new_list() {
        return List(*this);
    }
};

}
