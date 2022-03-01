/**
  * @file callable_style.h
  *
  * @brief Generates function signatures for static/member funcitons based on
  * the argument type.
  *
  * @author Jive Helix (jivehelix@gmail.com)
  * @date 11 Aug 2020
  * @copyright Jive Helix
  * Licensed under the MIT license. See LICENSE file.
**/

#pragma once

#include <type_traits>
#include "pex/detail/argument.h"


namespace pex
{

namespace detail
{


template<typename Observer, typename T>
using UnboundValueCallable =
    void (*)(Observer * const observer, typename Argument<T>::Type value);

template<typename Observer, typename T>
using BoundValueCallable =
    void (Observer::*)(typename Argument<T>::Type value);

// Use bound notification methods for all Observers except void.
// Note: If the compiler complains that your callback function takes the wrong
// number of arguments, it is possible that your Observer type is accidentally
// void.
template<typename Observer, typename T, typename = void>
struct CallableStyle;

template<typename Observer, typename T>
struct CallableStyle<Observer, T, std::enable_if_t<std::is_void_v<Observer>>>
{
    using Type = UnboundValueCallable<Observer, T>;
};

template<typename Observer, typename T>
struct CallableStyle<Observer, T, std::enable_if_t<!std::is_void_v<Observer>>>
{
    using Type = BoundValueCallable<Observer, T>;
};

template<typename Observer, typename T>
using ValueCallable = typename CallableStyle<Observer, T>::Type;

} // namespace detail

} // namespace pex
