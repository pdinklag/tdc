#pragma once

#include <concepts>
#include <cstddef>
#include <type_traits>
#include <utility>

namespace tdc {

template<typename T>
concept Arithmetic = std::is_arithmetic<T>::value;

template<typename T>
concept IndexAccess = requires(T a) {
    { a[std::declval<size_t>()] };
};

template<typename T>
concept SizedIndexAccess = 
    IndexAccess<T> &&
    requires(T a) {
        { a.size() } -> std::unsigned_integral;
    };

template<typename T, typename V>
concept IndexAccessTo = requires(T a) {
    { a[std::declval<size_t>()] } -> std::convertible_to<V>;
};

template<typename T, typename V>
concept SizedIndexAccessTo =
    IndexAccessTo<T, V> &&
    requires(T a) {
        { a.size() } -> std::unsigned_integral;
    };

template<typename T, typename V>
concept VectorCompilant = 
    SizedIndexAccess<T> &&
    std::semiregular<T> &&
    std::copy_constructible<T> &&
    std::move_constructible<T> &&
    std::constructible_from<T, size_t> &&
    requires(T a, V&& v) {
        { a.emplace_back(v) };
    } &&
    requires(T a, const V& v) {
        { a.push_back(v) };
    };

} // namespace tdc
