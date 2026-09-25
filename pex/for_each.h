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
    template<typename> typename Tailor
>
auto DeduceSchemaBase(const Schema<Tailor> &) -> Schema<Tailor>;


template<typename T, typename = void>
struct SchemaBase_
{
    using Type = decltype(
        DeduceSchemaBase(std::declval<const std::remove_cvref_t<T> &>()));
};


template<typename T>
using SchemaBase = decltype(
    DeduceSchemaBase(std::declval<const std::remove_cvref_t<T> &>()));


template<typename T>
concept HasSchemaBase = requires
{
    typename SchemaBase<T>;
    requires fields::CanReflectImpl<SchemaBase<T>>;
};


} // end namespace detail


} // end namespace pex


namespace fields
{


// Sometimes a struct can be both aggregate and have a SchemaBase.
// In those cases, always reflect the SchemaBase.
// fields::ReflectorTypeImpl already matches aggregate structs.
template<typename T>
struct ReflectorTypeImpl
<
    T,
    std::enable_if_t
    <
        !fields::DefinesReflector<T>
        && pex::detail::HasSchemaBase<T>
    >
>
{
    using Type = pex::detail::SchemaBase<T>;
};


} // end namespace fields
