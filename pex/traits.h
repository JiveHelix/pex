#pragma once


#include <type_traits>
#include "pex/model_value.h"
#include "pex/signal.h"


namespace pex
{


template<typename ...T>
struct IsModel_: std::false_type {};

template<typename ...T>
struct IsModel_<pex::model::Value_<T...>>: std::true_type {};

template<typename ...T>
inline constexpr bool IsModel = IsModel_<T...>::value;


template<typename Pex>
struct IsDirect_: std::false_type {};

template<typename Pex>
struct IsDirect_<pex::model::Direct<Pex>>: std::true_type {};

template<typename Pex>
inline constexpr bool IsDirect = IsDirect_<Pex>::value;


template<typename ...T>
struct IsControlBase_: std::false_type {};

template<typename ...T>
struct IsControlBase_<pex::control::Value_<T...>>: std::true_type {};

template<typename ...T>
inline constexpr bool IsControlBase = IsControlBase_<T...>::value;


template<template<typename...> typename Base, typename Derived>
struct IsBaseOf_
{
    template<typename ...T>
    static constexpr std::true_type  DoTest_(const Base<T...> *);
    static constexpr std::false_type DoTest_(...);
    using Type = decltype(DoTest_(std::declval<Derived *>()));
};

template<template<typename...> class Base, typename Derived>
using IsBaseOf = typename IsBaseOf_<Base, Derived>::Type;


template<typename T>
struct IsControl_: IsBaseOf<pex::control::Value_, T> {};

template<typename T>
inline constexpr bool IsControl = IsControl_<T>::value;


template<typename T, typename enable = void>
struct IsModelSignal_: std::false_type {};

template<typename T>
struct IsModelSignal_
<
    T,
    std::enable_if_t
    <
        std::is_same_v<T, pex::model::Signal>
    >
>: std::true_type {};

template<typename T>
inline constexpr bool IsModelSignal = IsModelSignal_<T>::value;


template<typename ...T>
struct IsControlSignal_: std::false_type {};

template<typename ...T>
struct IsControlSignal_<pex::control::Signal<T...>>: std::true_type {};

template<typename ...T>
inline constexpr bool IsControlSignal = IsControlSignal_<T...>::value;


template<typename T, typename enable = void>
struct IsSignal_: std::false_type {};

template<typename T>
struct IsSignal_
<
    T,
    std::enable_if_t
    <
        IsModelSignal<T> || IsControlSignal<T>
    >
>: std::true_type {};

template<typename T>
inline constexpr bool IsSignal = IsSignal_<T>::value;


/** If Pex is a pex::model::Value, it cannot be copied. Also, if it
 ** has a Filter with member functions, then allowing it to be copied
 ** breaks the ability to change the Filter instance. These Pex values must not
 ** be copied.
 **/
template<typename Pex, typename = void>
struct IsCopyable_: std::false_type {};


template<typename Pex>
struct IsCopyable_
<
    Pex,
    std::enable_if_t
    <
        !IsModel<Pex>
        && 
        !IsModelSignal<Pex>
        &&
        !detail::FilterIsMember
        <
            typename Pex::UpstreamType,
            typename Pex::Filter
        >
    >
>: std::true_type {};


template<typename Pex>
inline constexpr bool IsCopyable = IsCopyable_<Pex>::value;


/** 
 ** Copyable Upstream may be stored directly, else use Direct.
 **/
template<typename T, typename Enable = void>
struct UpstreamHolder_
{
    using Type = T;
};


template<typename T>
struct UpstreamHolder_
<
    T,
    std::enable_if_t<!IsCopyable<T>>
>
{
    using Type = ::pex::model::Direct<T>;
};


template<typename T>
using UpstreamHolderT = typename UpstreamHolder_<T>::Type;

/** UpstreamHolderT **/


/**
 ** When passed as a constructor argument, non-copyable types will be
 ** passed by reference. Otherwise, a copy will be made. This allows
 ** a control::Value to be an rvalue.
 **/
template<typename T, typename = void>
struct PexArgument_
{
    using Type = T;
};

template<typename T>
struct PexArgument_<T, std::enable_if_t<!IsCopyable<T>>>
{
    using Type = T &;
};


template<typename T>
using PexArgument = typename PexArgument_<T>::Type;

/** PexArgument **/


} // end namespace pex
