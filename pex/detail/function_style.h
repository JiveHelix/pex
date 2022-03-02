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
using FreeFunction =
    void (*)(Observer * const observer, ArgumentT<T> value);

template<typename Observer, typename T>
using MemberFunction =
    void (Observer::*)(ArgumentT<T> value);


template<typename Observer>
using SignalFreeFunction = void (*)(Observer * const observer);

template<typename Observer>
using SignalMemberFunction = void (Observer::*)();


// Use member function notification methods for all Observers except void.
// Note: If the compiler complains that your callback function takes the wrong
// number of arguments, it is possible that your Observer type is accidentally
// void.
template<typename Observer, typename T, typename = void>
struct FunctionStyle_;

template<typename Observer, typename T>
struct FunctionStyle_<Observer, T, std::enable_if_t<std::is_void_v<Observer>>>
{
    using Value = FreeFunction<Observer, T>;
};

template<typename Observer, typename T>
struct FunctionStyle_<Observer, T, std::enable_if_t<!std::is_void_v<Observer>>>
{
    using Value = MemberFunction<Observer, T>;
};

template<typename Observer, typename = void>
struct SignalFunctionStyle_;

template<typename Observer>
struct SignalFunctionStyle_
<
    Observer,
    std::enable_if_t<std::is_void_v<Observer>>
>
{
    using Signal = SignalFreeFunction<Observer>;
};

template<typename Observer>
struct SignalFunctionStyle_
<
    Observer,
    std::enable_if_t<!std::is_void_v<Observer>>
>
{
    using Signal = SignalMemberFunction<Observer>;
};

template<typename Observer, typename T>
using ValueFunctionStyle = typename FunctionStyle_<Observer, T>::Value;

template<typename Observer>
using SignalFunctionStyle = typename SignalFunctionStyle_<Observer>::Signal;




} // namespace detail

} // namespace pex
