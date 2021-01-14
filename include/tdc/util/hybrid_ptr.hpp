#pragma once

#include <cassert>

#include <limits>
#include <memory>
#include <utility>

namespace tdc {

/// \brief A unique pointer to either one of two object types.
///
/// This class owns an object of either type and identifies its type by using the pointer's least significant bit,
/// which is typically unused on any modern computer architecture.
/// Thanks to this fact, a hybrid pointer uses the same space as a usual pointer.
///
/// \tparam first_t the first type
/// \tparam second_t the second type
template<typename first_t, typename second_t>
class hybrid_ptr {
private:
    using ptr_int_t = unsigned long long;
    static_assert(sizeof(ptr_int_t) == sizeof(void*), "unsupported pointer size");
    
    static constexpr ptr_int_t MASK_SECOND = std::numeric_limits<ptr_int_t>::max() - 1;

    ptr_int_t m_ptr;

public:
    /// \brief Constructs a null pointer to neither type.
    hybrid_ptr() : m_ptr(0) {
    }
    
    hybrid_ptr(const hybrid_ptr& other) = delete;
    hybrid_ptr(hybrid_ptr&& other) {
        *this = std::move(other);
    }
    
    hybrid_ptr& operator=(const hybrid_ptr& other) = delete;
    hybrid_ptr& operator=(hybrid_ptr&& other) = default;

    /// \brief Initializes pointer to the first type of object, moving ownership to the hybrid pointer.
    hybrid_ptr(std::unique_ptr<first_t>&& ptr) {
        *this = std::move(ptr);
    }
    
    /// \brief Initializes pointer to the second type of object, moving ownership to the hybrid pointer.
    hybrid_ptr(std::unique_ptr<second_t>&& ptr) {
        *this = std::move(ptr);
    }
    
    ~hybrid_ptr() {
        reset();
    }
    
    /// \brief Reports whether the pointer is non-null.
    operator bool() const {
        return m_ptr;
    }
    
    /// \brief Reports whether the pointer is currently pointing to the first object type.
    ///
    /// Note that this does not test whether the pointer is non-null.
    bool is_first() const {
        assert(m_ptr);
        return !is_second();
    }

    /// \brief Reports whether the pointer is currently pointing to the first object type.
    ///
    /// Note that this does not test whether the pointer is non-null.    
    bool is_second() const {
        assert(m_ptr);
        return m_ptr & 1;
    }
    
    /// \brief Assigns a pointer to the first type of object.
    ///
    /// This will destroy the currently owned object, if any.
    hybrid_ptr& operator=(std::unique_ptr<first_t>&& ptr) {
        reset();
        m_ptr = (ptr_int_t)ptr.release();
        return *this;
    }
    
    /// \brief Assigns a pointer to the second type of object.
    ///
    /// This will destroy the currently owned object, if any.
    hybrid_ptr& operator=(std::unique_ptr<second_t>&& ptr) {
        reset();
        m_ptr = (ptr_int_t)ptr.release() | 1;
        return *this;
    }
    
    /// \brief Converts the pointer to one to the first type of object.
    ///
    /// Note that this does not test whether the pointer actually points to the correct type of object.
    /// If that is not the case, the pointer is invalid.
    first_t* as_first() {
        if(m_ptr) assert(is_first());
        return (first_t*)m_ptr;
    }

    /// \brief Converts the pointer to one to the first type of object.
    ///
    /// Note that this does not test whether the pointer actually points to the correct type of object.
    /// If that is not the case, the pointer is invalid.
    const first_t* as_first() const {
        if(m_ptr) assert(is_first());
        return (const first_t*)m_ptr;
    }
    
    /// \brief Converts the pointer to one to the first type of object.
    ///
    /// Note that this does not test whether the pointer actually points to the correct type of object.
    /// If that is not the case, the pointer is invalid.
    second_t* as_second() {
        if(m_ptr) assert(is_second());
        return (second_t*)(m_ptr & MASK_SECOND);
    }

    /// \brief Converts the pointer to one to the first type of object.
    ///
    /// Note that this does not test whether the pointer actually points to the correct type of object.
    /// If that is not the case, the pointer is invalid.
    const second_t* as_second() const {
        if(m_ptr) assert(is_second());
        return (const second_t*)(m_ptr & MASK_SECOND);
    }
    
    /// \brief Releases ownership of the pointer the first type of object.
    ///
    /// Note that this does not test whether the pointer actually points to the correct type of object.
    /// If that is not the case, the pointer is invalid.
    std::unique_ptr<first_t> release_as_first() {
        assert(m_ptr);
        assert(is_first());
        
        std::unique_ptr<first_t> ptr(as_first());
        m_ptr = 0;
        return ptr;
    }
    
    /// \brief Releases ownership of the pointer the second type of object.
    ///
    /// Note that this does not test whether the pointer actually points to the correct type of object.
    /// If that is not the case, the pointer is invalid.
    std::unique_ptr<second_t> release_as_second() {
        assert(m_ptr);
        assert(is_first());
        
        std::unique_ptr<second_t> ptr(as_second());
        m_ptr = 0;
        return ptr;
    }
    
    /// \brief Resets the pointer to a null pointer.
    ///
    /// This will destroy the currently owned object, if any.
    void reset() {
        if(m_ptr) {
            if(is_first()) {
                release_as_first();
                // at this point, the object's destructor has been called
            } else {
                release_as_second();
                // at this point, the object's destructor has been called
            }
            m_ptr = 0;
        }
    }
} __attribute__((__packed__));

}
