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
// #include <jive/optional.h>
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
struct GetterIsStatic_: std::false_type {};

template<typename T>
struct GetterIsStatic_<T, NoFilter, void>: std::false_type {};

template<typename T, typename Filter>
struct GetterIsStatic_
<
    T,
    Filter,
    std::enable_if_t
    <
        std::is_invocable_v
        <
            decltype(&Filter::Get),
            jive::RemoveOptional<T>
        >
    >
> : std::true_type {};

template<typename T, typename Filter>
inline constexpr bool GetterIsStatic = GetterIsStatic_<T, Filter>::value;


template<typename T, typename Filter, typename = void>
struct GetterIsMember_: std::false_type {};

template<typename T>
struct GetterIsMember_<T, NoFilter, void>: std::false_type {};

template<typename T, typename Filter>
struct GetterIsMember_
<
    T,
    Filter,
    std::enable_if_t
    <
        std::is_invocable_v
        <
            decltype(&Filter::Get), Filter, jive::RemoveOptional<T>
        >
    >
> : std::true_type {};

template<typename T, typename Filter>
inline constexpr bool GetterIsMember = GetterIsMember_<T, Filter>::value;


/** Filter can change the type of the value.
 ** I use the return value of the Get() method to deduce the converted type
 **
 ** The default is the unmodified type T.
 **/
template<typename T, typename Filter, typename = void>
struct FilteredType_
{
    using Type = T;
};

template<typename T>
struct FilteredType_<T, NoFilter, void>
{
    using Type = T;
};

template<typename T, typename Filter>
struct FilteredType_
<
    T,
    Filter,
    std::enable_if_t<GetterIsMember<T, Filter>>>
{
    using Type =
        jive::MatchOptional
        <
            T,
            std::invoke_result_t
            <
                decltype(&Filter::Get),
                Filter,
                jive::RemoveOptional<T>
            >
        >;
};

template<typename T, typename Filter>
struct FilteredType_<
    T,
    Filter,
    std::enable_if_t<GetterIsStatic<T, Filter>>>
{
    using Type =
        jive::MatchOptional
        <
            T,
            std::invoke_result_t
            <
                decltype(&Filter::Get), jive::RemoveOptional<T>
            >
        >;
};

template<typename T, typename Filter>
using FilteredType = typename FilteredType_<T, Filter>::Type;


/** Setter Checks
 **
 ** Set can be either static or member, and it may modify the type.
 ** Use the FilteredType helper class to select the correct argument to the
 ** Set function.
 **/
template<typename T, typename Filter, typename = void>
struct SetterIsStatic_: std::false_type {};

template<typename T>
struct SetterIsStatic_<T, NoFilter, void>: std::false_type {};

template<typename T, typename Filter>
struct SetterIsStatic_
<
    T,
    Filter,
    std::enable_if_t
    <
        std::is_invocable_r_v
        <
            jive::RemoveOptional<T>,
            decltype(&Filter::Set),
            jive::RemoveOptional<FilteredType<T, Filter>>
        >
    >
> : std::true_type {};

template<typename T, typename Filter>
inline constexpr bool SetterIsStatic = SetterIsStatic_<T, Filter>::value;


template<typename T, typename Filter, typename = void>
struct SetterIsMember_: std::false_type {};

template<typename T>
struct SetterIsMember_<T, NoFilter, void>: std::false_type {};

template<typename T, typename Filter>
struct SetterIsMember_
<
    T,
    Filter,
    std::enable_if_t
    <
        std::is_invocable_r_v
        <
            jive::RemoveOptional<T>,
            decltype(&Filter::Set),
            Filter,
            jive::RemoveOptional<FilteredType<T, Filter>>
        >
    >
> : std::true_type {};

template<typename T, typename Filter>
inline constexpr bool SetterIsMember = SetterIsMember_<T, Filter>::value;


/**
 ** Filter::Get can be a static method or a member method.
 **/
template<typename T, typename Filter, typename = void>
struct GetterIsValid_ : std::false_type {};

template<typename T>
struct GetterIsValid_<T, NoFilter, void> : std::false_type {};

template<typename T, typename Filter>
struct GetterIsValid_
<
    T,
    Filter,
    std::enable_if_t
    <
        (GetterIsStatic<T, Filter> || GetterIsMember<T, Filter>)
        && !std::is_same_v<FilteredType<T, Filter>, void>
    >
> : std::true_type {};

template<typename T, typename Filter>
inline constexpr bool GetterIsValid = GetterIsValid_<T, Filter>::value;


/**
 ** Filter::Set can be a static method or a member method.
 **/
template<typename T, typename Filter, typename = void>
struct SetterIsValid_ : std::false_type {};

template<typename T>
struct SetterIsValid_<T, NoFilter, void> : std::false_type {};

template<typename T, typename Filter>
struct SetterIsValid_
<
    T,
    Filter,
    std::enable_if_t
    <
        SetterIsStatic<T, Filter> || SetterIsMember<T, Filter>
    >
> : std::true_type {};

template<typename T, typename Filter>
inline constexpr bool SetterIsValid = SetterIsValid_<T, Filter>::value;


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
struct FilterIsStatic_ : std::false_type {};

/** For read-only interfaces, only the getter is checked. **/
template<typename T, typename Filter>
struct FilterIsStatic_
<
    T,
    Filter,
    GetTag,
    std::enable_if_t
    <
        GetterIsStatic<T, Filter>
    >
> : std::true_type {};

template<typename T, typename Filter>
struct FilterIsStatic_
<
    T,
    Filter,
    SetTag,
    std::enable_if_t
    <
        SetterIsStatic<T, Filter>
    >
> : std::true_type {};

/** Check both the getter and setter for settable interfaces. **/
template<typename T, typename Filter>
struct FilterIsStatic_
<
    T,
    Filter,
    GetAndSetTag,
    std::enable_if_t
    <
        GetterIsStatic<T, Filter>
        && SetterIsStatic<T, Filter>
    >
> : std::true_type {};

template<typename T, typename Filter, typename Access>
inline constexpr bool FilterIsStatic =
    FilterIsStatic_<T, Filter, Access>::value;


/**
 ** Checks whether Set or Get are member functions.
 **
 ** This is used to check whether a pointer to the Filter structure will be
 ** required.
 **/
template<typename T, typename Filter, typename = void>
struct FilterIsMember_: std::false_type {};

template<typename T, typename Filter>
struct FilterIsMember_
<
    T,
    Filter,
    std::enable_if_t
    <
        GetterIsMember<T, Filter> || SetterIsMember<T, Filter>
    >
> : std::true_type {};

template<typename T, typename Filter>
inline constexpr bool FilterIsMember = FilterIsMember_<T, Filter>::value;


template<typename Filter>
struct FilterIsNone_: std::false_type {};

template<>
struct FilterIsNone_<NoFilter>: std::true_type {};

template<typename Filter>
inline constexpr bool FilterIsNone = FilterIsNone_<Filter>::value;


template
<
    typename T,
    typename Filter,
    typename Access,
    typename = void
>
struct FilterIsNoneOrStatic_ : std::false_type {};

/** Filter can be void **/
template<typename T, typename Access>
struct FilterIsNoneOrStatic_<T, NoFilter, Access, void> : std::true_type {};

template<typename T, typename Filter, typename Access>
struct FilterIsNoneOrStatic_
<
    T,
    Filter,
    Access,
    std::enable_if_t<FilterIsStatic<T, Filter, Access>>
> : std::true_type {};

template<typename T, typename Filter, typename Access>
inline constexpr bool FilterIsNoneOrStatic =
    FilterIsNoneOrStatic_<T, Filter, Access>::value;



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
struct FilterIsValid_ : std::false_type {};

template<typename T, typename Filter>
struct FilterIsValid_
<
    T,
    Filter,
    GetTag,
    std::enable_if_t<GetterIsValid<T, Filter>>
> : std::true_type {};

template<typename T, typename Filter>
struct FilterIsValid_
<
    T,
    Filter,
    SetTag,
    std::enable_if_t<SetterIsValid<T, Filter>>
> : std::true_type {};

template<typename T, typename Filter>
struct FilterIsValid_
<
    T,
    Filter,
    GetAndSetTag,
    std::enable_if_t
    <
        GetterIsValid<T, Filter>
        && SetterIsValid<T, Filter>
    >
> : std::true_type {};

template<typename T, typename Filter, typename Access>
inline constexpr bool FilterIsValid = FilterIsValid_<T, Filter, Access>::value;


template
<
    typename T,
    typename Filter,
    typename Access,
    typename = void
>
struct FilterIsNoneOrValid_ : std::false_type {};

/** Filter can be void **/
template<typename T, typename Access>
struct FilterIsNoneOrValid_<T, NoFilter, Access, void> : std::true_type {};

template<typename T, typename Filter, typename Access>
struct FilterIsNoneOrValid_
<
    T,
    Filter,
    Access,
    std::enable_if_t<FilterIsValid<T, Filter, Access>>
> : std::true_type {};

template<typename T, typename Filter, typename Access>
inline constexpr bool FilterIsNoneOrValid =
    FilterIsNoneOrValid_<T, Filter, Access>::value;


} // namespace detail

} // namespace pex
