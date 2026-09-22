#pragma once


#include <fields/reflect.h>


namespace pex
{


namespace detail
{


// Deduce aggregate base class if possible

template
<
    template<template<typename> typename> typename Schema,
    template<typename> typename Selector
>
auto DeduceSchemaBase(const Schema<Selector> &) -> Schema<Selector>;

template<typename T>
concept DefinesSchemaBase = requires
{
    typename std::remove_cvref_t<T>::SchemaBase;
};


template<typename T, typename = void>
struct SchemaBase_
{
    using Type = decltype(
        DeduceSchemaBase(std::declval<const std::remove_cvref_t<T> &>()));
};


template<typename T>
struct SchemaBase_
<
    T,
    std::enable_if_t<DefinesSchemaBase<T>>
>
{
    using Type = typename std::remove_cvref_t<T>::SchemaBase;
};


template<typename T>
using SchemaBase = typename SchemaBase_<T>::Type;


template<typename T>
concept HasSchemaBase = requires
{
    typename SchemaBase<T>;
    requires std::is_aggregate_v<SchemaBase<T>>;
};


} // end namespace detail


} // end namespace pex



namespace fields
{


namespace detail
{


template<typename T>
constexpr auto && ForwardReflectable(T &&t)
{
    if constexpr (fields::CanReflect<T>)
    {
        return std::forward<T>(t);
    }
    else
    {
        static_assert(
            pex::detail::HasSchemaBase<T>,
            "Requires SchemaBase");

        using Type = std::remove_cvref_t<T>;

        using Base = std::conditional_t
            <
                std::is_const_v<std::remove_reference_t<T>>,
                const pex::detail::SchemaBase<Type>,
                pex::detail::SchemaBase<T>
            >;

        using Reference = std::conditional_t
            <
                std::is_lvalue_reference_v<T>,
                Base &,
                Base &&
            >;

        return static_cast<Reference>(t);
    }
}


} // end namespace detail


template<typename T, typename Function>
    requires(!CanReflect<T>)
void ForEach(T &&t, Function &&function)
{
    static_assert(pex::detail::HasSchemaBase<T>);

    ForEach(
        detail::ForwardReflectable(t),
        std::forward<Function>(function));
}


template<typename Left, typename Right, typename Function>
    requires(!CanReflect<Left> || !CanReflect<Right>)
void ForEachZip(Left &&left, Right &&right, Function &&function)
{
    static_assert(
        pex::detail::HasSchemaBase<Left> || CanReflect<Left>,
        "Left must be reflectable or have SchemaBase");

    static_assert(
        pex::detail::HasSchemaBase<Right> || CanReflect<Right>,
        "Right must be reflectable or have SchemaBase");

    ForEachZip(
        detail::ForwardReflectable(left),
        detail::ForwardReflectable(right),
        std::forward<Function>(function));
}


} // end namespace fields
