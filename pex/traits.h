#pragma once


#include <type_traits>
#include "pex/model_value.h"


namespace pex
{


template<typename ...T>
struct IsModel_: std::false_type {};

template<typename ...T>
struct IsModel_<pex::model::Value_<T...>>: std::true_type {};

template<typename ...T>
inline constexpr bool IsModel = IsModel_<T...>::value;


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
struct Upstream_
{
    using Type = T;
};


template<typename T>
struct Upstream_
<
    T,
    std::enable_if_t<!IsCopyable<T>>
>
{
    using Type = ::pex::model::Direct<T>;
};


template<typename T>
using UpstreamT = typename Upstream_<T>::Type;

/** UpstreamT **/


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
