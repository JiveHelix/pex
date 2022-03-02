/**
  * @file access_tag.h
  *
  * @brief Tags to specifiy access level for control values.
  *
  *
  * @author Jive Helix (jivehelix@gmail.com)
  * @date 14 Aug 2020
  * @copyright Jive Helix
  * Licensed under the MIT license. See LICENSE file.
**/

#pragma once


namespace pex
{


struct AccessTag {};
struct GetTag : AccessTag {};
struct SetTag : AccessTag {};
struct GetAndSetTag : GetTag, SetTag {};


template<typename Access, typename T, typename Enable = void>
struct HasAccess_: std::false_type {};


template<typename Access, typename T>
struct HasAccess_
<
    Access,
    T,
    std::enable_if_t<std::is_base_of_v<Access, T>>
>: std::true_type {};


template<typename Access, typename T>
inline constexpr bool HasAccess = HasAccess_<Access, T>::value;


} // namespace pex
