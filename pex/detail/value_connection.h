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

#include "pex/detail/connection.h"
#include "pex/detail/argument.h"
#include "pex/detail/function_style.h"


namespace pex
{

namespace detail
{


template<typename Observer, typename T>
class ValueConnection
    : public Connection<Observer, ValueFunctionStyle<Observer, T>>
{
public:
    using Type = T;
    using Base = Connection<Observer, ValueFunctionStyle<Observer, T>>;
    using Base::Base;

    void operator()(ArgumentT<T> value)
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
