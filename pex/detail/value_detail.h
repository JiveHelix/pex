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
