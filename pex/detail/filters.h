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
struct GetterIsStatic<T, NoFilter, void>: std::false_type {};

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
struct GetterIsMember<T, NoFilter, void>: std::false_type {};

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
struct FilteredType
{
    using Type = T;
};

template<typename T>
struct FilteredType<T, NoFilter, void>
{
    using Type = T;
};

template<typename T, typename Filter>
struct FilteredType
<
    T,
    Filter,
    std::enable_if_t<GetterIsMember<T, Filter>::value>>
{
    using Type = std::invoke_result_t<decltype(&Filter::Get), Filter, T>;
};

template<typename T, typename Filter>
struct FilteredType<
    T,
    Filter,
    std::enable_if_t<GetterIsStatic<T, Filter>::value>>
{
    using Type = std::invoke_result_t<decltype(&Filter::Get), T>;
};


/** Setter Checks
 **
 ** Set can be either static or member, and it may modify the type.
 ** Use the FilteredType helper class to select the correct argument to the
 ** Set function.
 **/
template<typename T, typename Filter, typename = void>
struct SetterIsStatic: std::false_type {};

template<typename T>
struct SetterIsStatic<T, NoFilter, void>: std::false_type {};

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
            typename FilteredType<T, Filter>::Type
        >
    >
> : std::true_type {};


template<typename T, typename Filter, typename = void>
struct SetterIsMember: std::false_type {};

template<typename T>
struct SetterIsMember<T, NoFilter, void>: std::false_type {};

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
            typename FilteredType<T, Filter>::Type
        >
    >
> : std::true_type {};


/**
 ** Filter::Get can be a static method or a member method.
 **/
template<typename T, typename Filter, typename = void>
struct GetterIsValid : std::false_type {};

template<typename T>
struct GetterIsValid<T, NoFilter, void> : std::false_type {};

template<typename T, typename Filter>
struct GetterIsValid
<
    T,
    Filter,
    std::enable_if_t
    <
        (GetterIsStatic<T, Filter>::value || GetterIsMember<T, Filter>::value)
        && !std::is_same_v<typename FilteredType<T, Filter>::Type, void>
    >
> : std::true_type {};


/**
 ** Filter::Set can be a static method or a member method.
 **/
template<typename T, typename Filter, typename = void>
struct SetterIsValid : std::false_type {};

template<typename T>
struct SetterIsValid<T, NoFilter, void> : std::false_type {};

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
 ** FilterIsStatic evaluates to true when both Get and Set methods are static.
 **/
template
<
    typename T,
    typename Filter,
    typename Access,
    typename = void
>
struct FilterIsStatic : std::false_type {};

/** For read-only interfaces, only the getter is checked. **/
template<typename T, typename Filter>
struct FilterIsStatic
<
    T,
    Filter,
    GetTag,
    std::enable_if_t
    <
        GetterIsStatic<T, Filter>::value
    >
> : std::true_type {};

template<typename T, typename Filter>
struct FilterIsStatic
<
    T,
    Filter,
    SetTag,
    std::enable_if_t
    <
        SetterIsStatic<T, Filter>::value
    >
> : std::true_type {};

/** Check both the getter and setter for settable interfaces. **/
template<typename T, typename Filter>
struct FilterIsStatic
<
    T,
    Filter,
    GetAndSetTag,
    std::enable_if_t
    <
        GetterIsStatic<T, Filter>::value
        && SetterIsStatic<T, Filter>::value
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


template
<
    typename T,
    typename Filter,
    typename Access,
    typename = void
>
struct FilterIsNoneOrStatic : std::false_type {};

/** Filter can be void **/
template<typename T, typename Access>
struct FilterIsNoneOrStatic<T, NoFilter, Access, void> : std::true_type {};

template<typename T, typename Filter, typename Access>
struct FilterIsNoneOrStatic
<
    T,
    Filter,
    Access,
    std::enable_if_t<FilterIsStatic<T, Filter, Access>::value>
> : std::true_type {};



/** Filter Validation
 **
 ** A Filter, if provided, must provide both Set and Get methods.
 ** These can be any mixture of static and member methods.
 **
 ** Get can return a different type than its argument, but Set must then accept
 ** the same type as returned by Get.
 **/

template
<
    typename T,
    typename Filter,
    typename Access,
    typename = void
>
struct FilterIsValid : std::false_type {};

template<typename T, typename Filter>
struct FilterIsValid
<
    T,
    Filter,
    GetTag,
    std::enable_if_t<GetterIsValid<T, Filter>::value>
> : std::true_type {};

template<typename T, typename Filter>
struct FilterIsValid
<
    T,
    Filter,
    SetTag,
    std::enable_if_t<SetterIsValid<T, Filter>::value>
> : std::true_type {};

template<typename T, typename Filter>
struct FilterIsValid
<
    T,
    Filter,
    GetAndSetTag,
    std::enable_if_t
    <
        GetterIsValid<T, Filter>::value
        && SetterIsValid<T, Filter>::value
    >
> : std::true_type {};


template
<
    typename T,
    typename Filter,
    typename Access,
    typename = void
>
struct FilterIsNoneOrValid : std::false_type {};

/** Filter can be void **/
template<typename T, typename Access>
struct FilterIsNoneOrValid<T, NoFilter, Access, void> : std::true_type {};

template<typename T, typename Filter, typename Access>
struct FilterIsNoneOrValid
<
    T,
    Filter,
    Access,
    std::enable_if_t<FilterIsValid<T, Filter, Access>::value>
> : std::true_type {};


} // namespace detail

} // namespace pex
