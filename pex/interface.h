/**
  * @file interface.h
  * 
  * @brief Declares templated helpers for generating interfaces.
  * 
  * @author Jive Helix (jivehelix@gmail.com)
  * @date 11 Mar 2022
  * @copyright Jive Helix
  * Licensed under the MIT license. See LICENSE file.
**/


#pragma once


#include "pex/value.h"
#include "pex/signal.h"


namespace pex
{

// Helpful templates for generating interfaces

template<typename T>
using Model = model::Value<T>;


template<typename Observer, template<typename> typename Upstream = pex::Model>
struct Control
{
    template<typename T>
    using Type = control::Value<Observer, Upstream<T>>;
};


struct MakeSignal;


template<
    typename T,
    typename ModelFilter_ = NoFilter,
    typename ControlAccess_ = GetAndSetTag>
struct Member
{
    using Type = T;
    using ModelFilter = ModelFilter_;
    using ControlAccess = ControlAccess_;
};

template<typename ...T>
struct IsMember_: std::false_type {};

template<typename ...T>
struct IsMember_<Member<T...>>: std::true_type {};

template<typename ...T>
inline constexpr bool IsMember = IsMember_<T...>::value;


template<typename T, typename = void>
struct ModelSelector_
{
    using Type = model::Value<T>;
};

template<typename T>
struct ModelSelector_<T, std::enable_if_t<std::is_same_v<T, MakeSignal>>>
{
    using Type = model::Signal;
};

template<typename T>
struct ModelSelector_<T, std::enable_if_t<IsMember<T>>>
{
    using Type = model::Value_<typename T::Type, typename T::ModelFilter>;
};

template<typename T>
using ModelSelector = typename ModelSelector_<T>::Type;


template<typename T, typename = void>
struct ControlSelector_
{
    template<typename Observer>
    using Type = control::Value<Observer, ModelSelector<T>>;
};

template<typename T>
struct ControlSelector_<T, std::enable_if_t<std::is_same_v<T, MakeSignal>>>
{
    template<typename Observer>
    using Type = control::Signal<Observer>;
};

template<typename T>
struct ControlSelector_<T, std::enable_if_t<IsMember<T>>>
{
    template<typename Observer>
    using Type = control::Value_
    <
        Observer,
        ModelSelector<T>,
        NoFilter,
        typename T::ControlAccess 
    >;
};

template<typename Observer>
struct ControlSelector
{
    template<typename T>
    using Type = typename ControlSelector_<T>::template Type<Observer>;
};


template<typename T, typename = void>
struct Identity_
{
    using Type = T;
};

template<typename T>
struct Identity_<T, std::enable_if_t<IsMember<T>>>
{
    using Type = typename T::Type;
};

template<typename T>
using Identity = typename Identity_<T>::Type;

} // end namespace pex
