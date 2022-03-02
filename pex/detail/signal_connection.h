/**
  * @file signal_notify.h
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
#include "pex/detail/connection.h"
#include "pex/detail/function_style.h"


namespace pex
{


namespace detail
{


template<typename Observer>
class SignalConnection
    : public Connection<Observer, SignalFunctionStyle<Observer>>
{
public:
    using Base = Connection<Observer, SignalFunctionStyle<Observer>>;
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
