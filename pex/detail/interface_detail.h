#pragma once


namespace pex
{


namespace detail
{


template<typename T, typename enable = void>
struct IsMakeSignal_: std::false_type {};

template<typename T>
struct IsMakeSignal_
<
    T,
    std::enable_if_t
    <
        std::is_same_v<T, MakeSignal>
    >
>: std::true_type {};


template<typename ...T>
struct IsMakeCustom_: std::false_type {};

template<template<typename> typename T, typename U>
struct IsMakeCustom_<MakeCustom<T<U>>>: std::true_type {};


template<typename ...T>
struct IsMakeGroup_: std::false_type {};

template
<
    typename G,
    typename M,
    template<typename> typename C,
    template<typename> typename T
>
struct IsMakeGroup_<MakeGroup<G, M, C, T>>: std::true_type {};


template<typename ...T>
struct IsFiltered_: std::false_type {};

template<typename ...T>
struct IsFiltered_<Filtered<T...>>: std::true_type {};


} // end namespace detail


template<typename T>
inline constexpr bool IsMakeSignal = detail::IsMakeSignal_<T>::value;

template<typename ...T>
inline constexpr bool IsMakeCustom = detail::IsMakeCustom_<T...>::value;

template<typename ...T>
inline constexpr bool IsMakeGroup = detail::IsMakeGroup_<T...>::value;

template<typename ...T>
inline constexpr bool IsFiltered = detail::IsFiltered_<T...>::value;

template<typename ...T>
inline constexpr bool IsRange = detail::IsFiltered_<T...>::value;


namespace detail
{


/***** ModelSelector *****/
template<typename T, typename = void>
struct ModelSelector_
{
    using Type = model::Value<T>;
};

template<typename T>
struct ModelSelector_<T, std::enable_if_t<IsMakeSignal<T>>>
{
    using Type = model::Signal;
};

template<typename T>
struct ModelSelector_<T, std::enable_if_t<IsFiltered<T>>>
{
    using Type = model::Value_<typename T::Type, typename T::ModelFilter>;
};

template<typename T>
struct ModelSelector_<T, std::enable_if_t<IsRange<T>>>
{
    using Type = model::Range<typename T::Type>;
};

template<typename T>
struct ModelSelector_<T, std::enable_if_t<IsMakeCustom<T>>>
{
    using Type = typename T::Custom;
};

template<typename T>
struct ModelSelector_<T, std::enable_if_t<IsMakeGroup<T>>>
{
    using Type = typename T::Model;
};


/***** ControlSelector *****/
template<typename T, typename = void>
struct ControlSelector_
{
    template<typename Observer>
    using Type = control::Value<Observer, typename ModelSelector_<T>::Type>;
};

template<typename T>
struct ControlSelector_<T, std::enable_if_t<IsMakeSignal<T>>>
{
    template<typename Observer>
    using Type = control::Signal<Observer>;
};

template<typename T>
struct ControlSelector_<T, std::enable_if_t<IsFiltered<T>>>
{
    template<typename Observer>
    using Type = control::Value_
    <
        Observer,
        typename ModelSelector_<T>::Type,
        NoFilter,
        typename T::ControlAccess 
    >;
};

template<typename T>
struct ControlSelector_<T, std::enable_if_t<IsRange<T>>>
{
    template<typename Observer>
    using Type = control::Value_
    <
        Observer,
        typename ModelSelector_<T>::Type,
        NoFilter,
        typename T::ControlAccess 
    >;
};

template<typename T>
struct ControlSelector_<T, std::enable_if_t<IsMakeCustom<T>>>
{
    template<typename Observer>
    using Type = typename T::template Control<Observer>;
};

template<typename T>
struct ControlSelector_<T, std::enable_if_t<IsMakeGroup<T>>>
{
    template<typename Observer>
    using Type = typename T::template Control<Observer>;
};


/***** TerminusSelector *****/
template<typename T, typename = void>
struct TerminusSelector_
{
    template<typename Observer>
    using Type = Terminus
    <
        Observer,
        typename ControlSelector_<T>::template Type<Observer>
    >;
};

template<typename T>
struct TerminusSelector_<T, std::enable_if_t<IsMakeCustom<T>>>
{
    template<typename Observer>
    using Type = Terminus
    <
        Observer,
        typename T::template Control<Observer>
    >;
};

template<typename T>
struct TerminusSelector_<T, std::enable_if_t<IsMakeGroup<T>>>
{
    template<typename Observer>
    using Type = typename T::template Terminus<Observer>;
};


/***** Identity *****/
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
        IsFiltered<T> || IsMakeCustom<T> || IsMakeGroup<T>
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


} // end namespace detail


} // end namespace pex
