/**
  * @file value_detail.h
  *
  * @brief Provides implementation details for pex/value.h.
  *
  * @author Jive Helix (jivehelix@gmail.com)
  * @date 11 Aug 2020
  * @copyright Jive Helix
  * Licensed under the MIT license. See LICENSE file.
**/

#pragma once

#include <type_traits>

#include "pex/detail/notify.h"
#include "pex/detail/value_notify.h"
#include "pex/detail/implements_connect.h"
#include "pex/detail/filters.h"


namespace pex
{

namespace detail
{


template<typename T, typename = void>
struct DefinesType: std::false_type {};


template<typename T>
struct DefinesType<T, std::void_t<typename T::Type>>
    : std::true_type {};


} // namespace detail

} // namespace pex


#ifndef NDEBUG
#define NOT_NULL(pointer)                                     \
    if (pointer == nullptr)                                   \
    {                                                         \
        throw std::logic_error(#pointer " must not be NULL"); \
    }

#else
#define NOT_NULL(pointer)
#endif
