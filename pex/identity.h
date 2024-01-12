#pragma once


#include "pex/interface.h"


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
        || IsMakeSelect<T>
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
        IsMakeList<T> || IsMakePolyList<T>
    >
>
{
    // Recursively look up the list type.
    using Type = std::vector<typename Identity_<typename T::MemberType>::Type>;
};


} // end namespace detail


template<typename T>
using Identity = typename detail::Identity_<T>::Type;


} // end namespace pex
