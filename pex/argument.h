/**
  * @file argument.h
  *
  * @brief Selects pass-by-value for plain-old data types, and const reference
  * for everything else.
  *
  * @author Jive Helix (jivehelix@gmail.com)
  * @date 07 Aug 2020
  * @copyright Jive Helix
  * Licensed under the MIT license. See LICENSE file.
**/

#pragma once

#include <type_traits>

namespace pex
{

namespace detail
{

// Set and Notify methods will use pass-by-value when T is an integral or
// floating-point type. Other types, like std::string will be passed by const
// reference to avoid unnecessary copying.
template<typename T, typename = void>
struct Argument_;


template<>
struct Argument_<void, void>
{

};


template<typename T>
struct Argument_
<
    T,
    std::enable_if_t
    <
        std::is_arithmetic_v<T> || std::is_enum_v<T>
    >
>
{
    using Type = T;
};

template<typename T>
struct Argument_
<
    T,
    std::enable_if_t
    <
        !(std::is_arithmetic_v<T> || std::is_enum_v<T>)
    >
>
{
    using Type = const T &;
};

} // namespace detail


template<typename T>
using Argument = typename detail::Argument_<T>::Type;


} // namespace pex
