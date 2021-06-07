#pragma once

#include <tdc/util/type_list.hpp>

namespace tdc {
namespace framework {

template<template<typename...> typename A, typename ParamsTl, typename... Tls>
struct _crossproduct;

// use this! :-)
template<template<typename...> typename A, typename... Tls>
using crossproduct = _crossproduct<A, tl::empty, Tls...>::type_list;

template<template<typename...> typename A, typename... Params>
struct _crossproduct<A, tl::list<Params...>> {
    using type_list = tl::list<A<Params...>>;
};

template<template<typename...> typename A, typename ParamsTl, typename... RemainingTls>
struct _crossproduct<A, ParamsTl, tl::empty, RemainingTls...> {
    using type_list = tl::empty; // cross product of nothing with something
};

template<template<typename...> typename A, typename... Params, typename Head, typename... Tail, typename... RemainingTls>
struct _crossproduct<A, tl::list<Params...>, tl::list<Head, Tail...>, RemainingTls...> {
    using TakeHead    = _crossproduct<A, tl::list<Params..., Head>, RemainingTls...>::type_list;
    using ProcessTail = _crossproduct<A, tl::list<Params...>, tl::list<Tail...>, RemainingTls...>::type_list;
    using type_list = tl::concat<TakeHead, ProcessTail>;
};

}} // namespace tdc::framework
