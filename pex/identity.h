#pragma once


#include "pex/interface.h"
#include "pex/poly.h"


namespace pex
{


namespace detail
{


template<typename T, typename = void>
struct Identity_
{
    using Type = T;
};

template<typename T>
struct Identity_
<
    T,
    std::enable_if_t
    <
        IsFiltered<T>
        || IsMakeCustom<T>
        || IsGroup<T>
        || IsMakeRange<T>
    >
>
{
    using Type = typename T::Type;
};


template<typename T>
struct Identity_<T, std::enable_if_t<IsMakeSignal<T>>>
{
    using Type = pex::DescribeSignal;
};


template<typename T>
struct Identity_
<
    T,
    std::enable_if_t
    <
        IsMakeList<T>
    >
>
{
    // Recursively look up the list type.
    using Type = std::vector<typename Identity_<typename T::MemberType>::Type>;
};


template<typename T>
struct Identity_
<
    T,
    std::enable_if_t
    <
        IsMakePolyList<T>
    >
>
{
    // Recursively look up the list type.
    using Type = std::vector<::pex::poly::Value<typename T::Supers::ValueBase>>;
};


template<typename T>
struct Identity_
<
    T,
    std::enable_if_t
    <
        IsMakeSelect<T>
    >
>
{
    using Type = typename T::Type::Type;
};


} // end namespace detail


template<typename T>
using Identity = typename detail::Identity_<T>::Type;


} // end namespace pex
