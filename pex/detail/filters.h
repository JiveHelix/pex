/**
  * @file filters.h
  *
  * @brief Implements utilities for using and verifying filters.
  *
  * @author Jive Helix (jivehelix@gmail.com)
  * @date 11 Aug 2020
  * @copyright Jive Helix
  * Licensed under the MIT license. See LICENSE file.
**/

#pragma once

#include <type_traits>
#include "pex/access_tag.h"

namespace pex
{

namespace detail
{

/**
 ** Getter Checks
 **
 ** Filter::Get can be either static or member, and the return type may
 ** differ from the argument type.
 **/
template<typename T, typename Filter, typename = void>
struct GetterIsStatic: std::false_type {};

template<typename T>
struct GetterIsStatic<T, void, void>: std::false_type {};

template<typename T, typename Filter>
struct GetterIsStatic
<
    T,
    Filter,
    std::enable_if_t<std::is_invocable_v<decltype(&Filter::Get), T>>
> : std::true_type {};


template<typename T, typename Filter, typename = void>
struct GetterIsMember: std::false_type {};

template<typename T>
struct GetterIsMember<T, void, void>: std::false_type {};

template<typename T, typename Filter>
struct GetterIsMember
<
    T,
    Filter,
    std::enable_if_t
    <
        std::is_invocable_v<decltype(&Filter::Get), Filter, T>
    >
> : std::true_type {};


/** Filter can change the type of the value.
 ** I use the return value of the Get() method to deduce the converted type
 **
 ** The default is the unmodified type T.
 **/
template<typename T, typename Filter, typename = void>
struct FilterType
{
    using Type = T;
};

template<typename T>
struct FilterType<T, void, void>
{
    using Type = T;
};

template<typename T, typename Filter>
struct FilterType<T, Filter, std::enable_if_t<GetterIsMember<T, Filter>::value>>
{
    using Type = std::invoke_result_t<decltype(&Filter::Get), Filter, T>;
};

template<typename T, typename Filter>
struct FilterType<T, Filter, std::enable_if_t<GetterIsStatic<T, Filter>::value>>
{
    using Type = std::invoke_result_t<decltype(&Filter::Get), T>;
};


/** Setter Checks
 **
 ** Set can be either static or member, and it may modify the type.
 ** Use the FilterType helper class to select the correct argument to the
 ** Set function.
 **/
template<typename T, typename Filter, typename = void>
struct SetterIsStatic: std::false_type {};

template<typename T>
struct SetterIsStatic<T, void, void>: std::false_type {};

template<typename T, typename Filter>
struct SetterIsStatic
<
    T,
    Filter,
    std::enable_if_t
    <
        std::is_invocable_r_v
        <
            T,
            decltype(&Filter::Set),
            typename FilterType<T, Filter>::Type
        >
    >
> : std::true_type {};


template<typename T, typename Filter, typename = void>
struct SetterIsMember: std::false_type {};

template<typename T>
struct SetterIsMember<T, void, void>: std::false_type {};

template<typename T, typename Filter>
struct SetterIsMember
<
    T,
    Filter,
    std::enable_if_t
    <
        std::is_invocable_r_v
        <
            T,
            decltype(&Filter::Set),
            Filter,
            typename FilterType<T, Filter>::Type
        >
    >
> : std::true_type {};


/**
 ** Filter::Get can be a static method or a member method.
 **/
template<typename T, typename Filter, typename = void>
struct GetterIsValid : std::false_type {};

template<typename T>
struct GetterIsValid<T, void, void> : std::false_type {};

template<typename T, typename Filter>
struct GetterIsValid
<
    T,
    Filter,
    std::enable_if_t
    <
        (GetterIsStatic<T, Filter>::value || GetterIsMember<T, Filter>::value)
        && !std::is_same_v<typename FilterType<T, Filter>::Type, void>
    >
> : std::true_type {};


/**
 ** Filter::Set can be a static method or a member method.
 **/
template<typename T, typename Filter, typename = void>
struct SetterIsValid : std::false_type {};

template<typename T>
struct SetterIsValid<T, void, void> : std::false_type {};

template<typename T, typename Filter>
struct SetterIsValid
<
    T,
    Filter,
    std::enable_if_t
    <
        SetterIsStatic<T, Filter>::value || SetterIsMember<T, Filter>::value
    >
> : std::true_type {};


/**
 ** Checks whether Set or Get are member functions.
 **
 ** This is used to check whether a pointer to the Filter structure will be
 ** required.
 **/
template<typename T, typename Filter, typename = void>
struct FilterIsMember: std::false_type {};

template<typename T, typename Filter>
struct FilterIsMember
<
    T,
    Filter,
    std::enable_if_t
    <
        GetterIsMember<T, Filter>::value || SetterIsMember<T, Filter>::value
    >
> : std::true_type {};


/** Model filter validation
 **
 ** Model filters only use the Set method to filter on input.
 **/
template<typename T, typename Filter, typename = void>
struct ModelFilterIsVoidOrValid: std::false_type {};

/** Filter can be void **/
template<typename T, typename Filter>
struct ModelFilterIsVoidOrValid
<
    T,
    Filter,
    std::enable_if_t<std::is_same_v<Filter, void>>
> : std::true_type {};

/** Filter::Set can be normal function, or a function that takes a pointer
 ** to Filter.
 **/
template<typename T, typename Filter>
struct ModelFilterIsVoidOrValid
<
    T,
    Filter,
    std::enable_if_t<SetterIsValid<T, Filter>::value>
> : std::true_type {};


/** Interface filter validation
 **
 ** Interface filters, if provided, must provide both Set and Get methods.
 ** These can be any mixture of static and member methods.
 **/

template<typename T, typename Filter, typename Access, typename = void>
struct InterfaceFilterIsValid : std::false_type {};

template<typename T, typename Filter, typename Access>
struct InterfaceFilterIsValid
<
    T,
    Filter,
    Access,
    std::enable_if_t
    <
        std::is_same_v<Access, GetOnlyTag>
        && GetterIsValid<T, Filter>::value
    >
> : std::true_type {};

template<typename T, typename Filter, typename Access>
struct InterfaceFilterIsValid
<
    T,
    Filter,
    Access,
    std::enable_if_t
    <
        std::is_same_v<Access, GetAndSetTag>
        && GetterIsValid<T, Filter>::value
        && SetterIsValid<T, Filter>::value
    >
> : std::true_type {};


template<typename T, typename Filter, typename Access, typename = void>
struct InterfaceFilterIsVoidOrValid : std::false_type {};

/** Filter can be void **/
template<typename T, typename Access>
struct InterfaceFilterIsVoidOrValid<T, void, Access, void> : std::true_type {};

template<typename T, typename Filter, typename Access>
struct InterfaceFilterIsVoidOrValid
<
    T,
    Filter,
    Access,
    std::enable_if_t<InterfaceFilterIsValid<T, Filter, Access>::value>
> : std::true_type {};


} // namespace detail

} // namespace pex
