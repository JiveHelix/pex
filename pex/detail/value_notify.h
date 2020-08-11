/**
  * @file value_notify.h
  *
  * @brief Implements Notification functor for both static and member
  * callbacks.
  *
  * @author Jive Helix (jivehelix@gmail.com)
  * @date 11 Aug 2020
  * @copyright Jive Helix
  * Licensed under the MIT license. See LICENSE file.
**/

#pragma once

#include "pex/detail/notify.h"
#include "pex/detail/argument.h"
#include "pex/detail/callable_style.h"


namespace pex
{

namespace detail
{


template<typename Observer, typename T>
using ValueCallable = typename CallableStyle<Observer, T>::Type;

template<typename Observer, typename T>
class ValueNotify: public Notify_<Observer, ValueCallable<Observer, T>>
{
public:
    using Type = T;
    using Base = Notify_<Observer, ValueCallable<Observer, T>>;
    using argumentType = typename Argument<T>::Type;
    using Base::Base;

    void operator()(typename Argument<T>::Type value)
    {
        if constexpr(Base::IsMemberFunction)
        {
            static_assert(
                !std::is_same_v<Observer, void>,
                "Cannot call member function on void type.");

            (this->observer_->*(this->callable_))(value);
        }
        else
        {
            this->callable_(this->observer_, value);
        }
    }
};


} // namespace detail

} // namespace pex
