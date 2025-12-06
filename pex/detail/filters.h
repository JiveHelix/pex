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
#include <jive/optional.h>
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

template<typename T, typename Filter>
concept MemberGetterTakesOptional =
    requires(Filter f)
    {
        requires std::is_member_function_pointer_v<decltype(&Filter::Get)>;

        { std::invoke(&Filter::Get, f, std::declval<jive::MakeOptional<T>>()) };
    };

template<typename T, typename Filter>
concept MemberGetterRequiresType =
    requires(T t, Filter f)
    {
        requires !MemberGetterTakesOptional<T, Filter>;
        requires std::is_member_function_pointer_v<decltype(&Filter::Get)>;

        { std::invoke(&Filter::Get, f, t) };
    };

template<typename T, typename Filter>
concept StaticGetterTakesOptional =
    requires
    {
        { Filter::Get(std::declval<jive::MakeOptional<T>>()) };
    };

template<typename T, typename Filter>
concept StaticGetterRequiresType =
    requires(T t)
    {
        requires !StaticGetterTakesOptional<T, Filter>;

        { Filter::Get(t) };
    };

template<typename T, typename Filter>
concept GetterIsMember =
    MemberGetterRequiresType<T, Filter> || MemberGetterTakesOptional<T, Filter>;


template<typename T, typename Filter>
concept GetterIsStatic =
    StaticGetterRequiresType<T, Filter> || StaticGetterTakesOptional<T, Filter>;


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
    std::enable_if_t<MemberGetterTakesOptional<T, Filter>>>
{
    using Type =
        std::invoke_result_t
        <
            decltype(&Filter::Get),
            Filter,
            T
        >;
};


template<typename T, typename Filter>
struct FilteredType_
<
    T,
    Filter,
    std::enable_if_t<MemberGetterRequiresType<T, Filter>>>
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
    std::enable_if_t<StaticGetterTakesOptional<T, Filter>>>
{
    using Type = std::invoke_result_t<decltype(&Filter::Get), T>;
};


template<typename T, typename Filter>
struct FilteredType_<
    T,
    Filter,
    std::enable_if_t<StaticGetterRequiresType<T, Filter>>>
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
 ** If it modifies the type, the argument to the set function must match the
 ** return value of the Get function. If there is no get function, Set must not
 ** modify the type.
 **
 ** Use the FilteredType helper class to select the correct argument to the
 ** Set function.
 **/

template<typename T, typename Filter>
concept MemberSetterTakesOptional =
    requires(Filter f)
    {
        requires std::is_member_function_pointer_v<decltype(&Filter::Set)>;

        {
            std::invoke(
                &Filter::Set,
                f,
                std::declval
                <
                    jive::MakeOptional<FilteredType<T, Filter>>
                >())
        } -> std::same_as<jive::MakeOptional<T>>;
    };

template<typename T, typename Filter>
concept MemberSetterRequiresType =
    requires(Filter f, FilteredType<T, Filter> filtered)
    {
        requires !MemberSetterTakesOptional<T, Filter>;
        requires std::is_member_function_pointer_v<decltype(&Filter::Set)>;


        { std::invoke(&Filter::Set, f, filtered) } -> std::same_as<T>;
    };

template<typename T, typename Filter>
concept StaticSetterTakesOptional =
    requires(jive::MakeOptional<FilteredType<T, Filter>> filtered)
    {
        { Filter::Set(filtered) } -> std::same_as<jive::MakeOptional<T>>;
    };

template<typename T, typename Filter>
concept StaticSetterRequiresType =
    requires(FilteredType<T, Filter> filtered)
    {
        requires !StaticSetterTakesOptional<T, Filter>;

        { Filter::Set(filtered) } -> std::same_as<T>;
    };

template<typename T, typename Filter>
concept SetterIsMember =
    MemberSetterRequiresType<T, Filter> || MemberSetterTakesOptional<T, Filter>;


template<typename T, typename Filter>
concept SetterIsStatic =
    StaticSetterRequiresType<T, Filter> || StaticSetterTakesOptional<T, Filter>;


/**
 ** Filter::Get can be a static method or a member method.
 **/
template<typename T, typename Filter>
concept GetterIsValid =
    (GetterIsMember<T, Filter> || GetterIsStatic<T, Filter>)
    && !std::same_as<FilteredType<T, Filter>, void>;


/**
 ** Filter::Set can be a static method or a member method.
 **/
template<typename T, typename Filter>
concept SetterIsValid =
    (SetterIsMember<T, Filter> || SetterIsStatic<T, Filter>);



template<typename Filter>
concept FilterIsNone = std::derived_from<Filter, NoFilter>;


template<typename T, typename Filter>
concept FilterIsMember = GetterIsMember<T, Filter> || SetterIsMember<T, Filter>;


/**
 ** FilterIsStatic evaluates to true when both Get and Set methods are static.
 **/
template<typename T, typename Filter>
concept FilterIsStatic = !FilterIsNone<Filter> && !FilterIsMember<T, Filter>;


template<typename T, typename Filter>
concept FilterIsNoneOrStatic =
    FilterIsNone<Filter> || FilterIsStatic<T, Filter>;


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
    typename Access
>
concept FilterIsNoneOrValid =
    FilterIsValid<T, Filter, Access> || FilterIsNone<Filter>;


template<typename T, typename Filter, typename Derived, typename = void>
struct PlainGetFilter
{
    using Type = FilteredType<T, Filter>;
    using GetType = jive::RemoveOptional<Type>;
    using SetType = jive::RemoveOptional<T>;

    static GetType Get(pex::Argument<SetType> value)
    {
        auto result = Filter::Get(value);

        if constexpr (jive::IsOptional<Type>)
        {
            assert(result);

            return *result;
        }
        else
        {
            return result;
        }
    }
};


template<typename T, typename Filter, typename Derived>
struct PlainGetFilter
<
    T,
    Filter,
    Derived,
    std::enable_if_t<FilterIsMember<T, Filter>>
>
{
    using Type = FilteredType<T, Filter>;
    using GetType = jive::RemoveOptional<Type>;
    using SetType = jive::RemoveOptional<T>;

    GetType Get(pex::Argument<SetType> value) const
    {
        auto result = static_cast<const Derived *>(this)->filter_.Get(value);

        if constexpr (jive::IsOptional<Type>)
        {
            assert(result);

            return *result;
        }
        else
        {
            return result;
        }
    }
};


template<typename T, typename Filter, typename Derived, typename = void>
struct PlainSetFilter
{
    using Type = FilteredType<T, Filter>;
    using GetType = jive::RemoveOptional<Type>;
    using SetType = jive::RemoveOptional<T>;

    static SetType Set(pex::Argument<GetType> value)
    {
        auto result = Filter::Set(value);

        if constexpr (jive::IsOptional<Type>)
        {
            assert(result);

            return *result;
        }
        else
        {
            return result;
        }
    }
};


template<typename T, typename Filter, typename Derived>
struct PlainSetFilter
<
    T,
    Filter,
    Derived,
    std::enable_if_t<FilterIsMember<T, Filter>>
>
{
    using Type = FilteredType<T, Filter>;
    using GetType = jive::RemoveOptional<Type>;
    using SetType = jive::RemoveOptional<T>;

    SetType Set(pex::Argument<GetType> value) const
    {
        auto result = static_cast<const Derived *>(this)->filter_.Set(value);

        if constexpr (jive::IsOptional<Type>)
        {
            assert(result);

            return *result;
        }
        else
        {
            return result;
        }
    }
};


template<typename T, typename Filter, typename Access, typename = void>
struct PlainFilter: public NoFilter
{

};


template<typename T, typename Filter, typename Access>
struct PlainFilter
<
    T,
    Filter,
    Access,
    std::enable_if_t
    <
        std::same_as<Access, GetTag>
        && !std::same_as<Filter, NoFilter>
    >
>
    : public PlainGetFilter
    <
        T,
        Filter,
        PlainFilter<T, Filter, Access>
    >
{
    PlainFilter()
        :
        filter_{}
    {

    }

    PlainFilter(const Filter &filter)
        :
        filter_(filter)
    {

    }

private:
    Filter filter_;

    friend struct PlainGetFilter
    <
        T,
        Filter,
        PlainFilter<T, Filter, Access>
    >;
};


template<typename T, typename Filter, typename Access>
struct PlainFilter
<
    T,
    Filter,
    Access,
    std::enable_if_t
    <
        std::same_as<Access, SetTag>
        && !std::same_as<Filter, NoFilter>
    >
>
    : public PlainSetFilter
    <
        T,
        Filter,
        PlainFilter<T, Filter, Access>
    >
{
    PlainFilter()
        :
        filter_{}
    {

    }

    PlainFilter(const Filter &filter)
        :
        filter_(filter)
    {

    }

private:
    Filter filter_;

    friend struct PlainSetFilter
    <
        T,
        Filter,
        PlainFilter<T, Filter, Access>
    >;
};


template<typename T, typename Filter, typename Access>
struct PlainFilter
<
    T,
    Filter,
    Access,
    std::enable_if_t
    <
        std::same_as<Access, GetAndSetTag>
        && !std::same_as<Filter, NoFilter>
    >
>
    :
    public PlainGetFilter
    <
        T,
        Filter,
        PlainFilter<T, Filter, Access>
    >,
    public PlainSetFilter
    <
        T,
        Filter,
        PlainFilter<T, Filter, Access>
    >
{
public:
    PlainFilter()
        :
        filter_{}
    {

    }

    PlainFilter(const Filter &filter)
        :
        filter_(filter)
    {

    }

private:
    Filter filter_;

    friend struct PlainGetFilter
    <
        T,
        Filter,
        PlainFilter<T, Filter, Access>
    >;

    template<typename, typename, typename, typename>
    friend struct PlainSetFilter;
};


} // namespace detail


} // namespace pex
