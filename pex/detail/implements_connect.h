/**
  * @file implements_connect.h
  *
  * @brief Provides static checks for Connect and Disconnect methods.
  *
  * @author Jive Helix (jivehelix@gmail.com)
  * @date 11 Aug 2020
  * @copyright Jive Helix
  * Licensed under the MIT license. See LICENSE file.
**/

#pragma once

#include "pex/detail/callable_style.h"

namespace pex
{

namespace detail
{

template<typename T, typename = void>
struct ImplementsConnect: std::false_type {};

template<typename T>
struct ImplementsConnect
<
    T,
    std::enable_if_t
    <
        std::is_invocable_v
        <
            decltype(&T::Connect),
            T,
            void * const,
            UnboundValueCallable<void, typename T::Type>
        >
    >
> : std::true_type {};


template<typename T, typename = void>
struct ImplementsDisconnect: std::false_type {};

template<typename T>
struct ImplementsDisconnect
<
    T,
    std::enable_if_t
    <
        std::is_invocable_v
        <
            decltype(&T::Disconnect),
            T,
            void * const
        >
    >
> : std::true_type {};

} // namespace detail

} // namespace pex
