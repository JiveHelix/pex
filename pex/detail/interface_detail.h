#pragma once


namespace pex
{


namespace detail
{


template<typename T, typename enable = void>
struct IsMakeSignal_: std::false_type {};

template<typename T>
struct IsMakeSignal_
<
    T,
    std::enable_if_t
    <
        std::is_same_v<T, MakeSignal>
    >
>: std::true_type {};


template<typename ...T>
struct IsMakeCustom_: std::false_type {};

template<template<typename> typename T, typename U>
struct IsMakeCustom_<MakeCustom<T<U>>>: std::true_type {};


template<typename ...T>
struct IsMakeGroup_: std::false_type {};

template
<
    typename G,
    typename M,
    template<typename> typename C,
    template<typename> typename T
>
struct IsMakeGroup_<MakeGroup<G, M, C, T>>: std::true_type {};


template<typename ...T>
struct IsFiltered_: std::false_type {};

template<typename ...T>
struct IsFiltered_<Filtered<T...>>: std::true_type {};


template<typename ...T>
struct IsMakeRange_: std::false_type {};

template<typename ...T>
struct IsMakeRange_<MakeRange<T...>>: std::true_type {};


} // end namespace detail


} // end namespace pex
