#pragma once


#include "pex/interface.h"
#include "pex/traits.h"
#include "pex/value_wrapper.h"


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
        || IsDefineNodes<T>
        || IsGroup<T>
        || IsList<T>
        || IsMakeRange<T>
    >
>
{
    using Type = typename T::Type;
};


template<typename T>
struct Identity_<T, std::enable_if_t<IsDerivedGroup<T>>
>
{
    using Type = typename T::DerivedValue;
};


template<typename T>
struct Identity_<T, std::enable_if_t<IsMakePoly<T>>
>
{
    using Type = poly::ValueWrapperTemplate<typename T::Supers::ValueBase>;
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
