/**
  * @file signal_detail.h
  *
  * @brief Implementation details for Signal.
  *
  * @author Jive Helix (jivehelix@gmail.com)
  * @date 11 Aug 2020
  * @copyright Jive Helix
  * Licensed under the MIT license. See LICENSE file.
**/

#pragma once

#include <type_traits>
#include "pex/detail/notify.h"


namespace pex
{

namespace detail
{

template<typename Observer>
using UnboundSignalCallable = void (*)(Observer * const observer);

template<typename Observer>
using BoundSignalCallable = void (Observer::*)();

// Use bound notification methods for all Observers except void.
template<typename Observer, typename = void>
struct SignalCallableStyle;

template<typename Observer>
struct SignalCallableStyle
<
    Observer,
    std::enable_if_t<std::is_void_v<Observer>>
>
{
    using type = detail::UnboundSignalCallable<Observer>;
};

template<typename Observer>
struct SignalCallableStyle
<
    Observer,
    std::enable_if_t<!std::is_void_v<Observer>>
>
{
    using type = detail::BoundSignalCallable<Observer>;
};

template<typename Observer>
using SignalCallable = typename SignalCallableStyle<Observer>::type;


template<typename Observer>
class SignalNotify: public Notify_<Observer, SignalCallable<Observer>>
{
public:
    using Base = Notify_<Observer, SignalCallable<Observer>>;
    using Base::Base;

    void operator()()
    {
        if constexpr(Base::IsMemberFunction)
        {
            static_assert(
                !std::is_same_v<Observer, void>,
                "Cannot call member function on void type.");

            (this->observer_->*(this->callable_))();
        }
        else
        {
            this->callable_(this->observer_);
        }
    }
};

} // namespace detail

} // namespace pex
