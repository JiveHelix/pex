/**
  * @file access_tag.h
  *
  * @brief Tags to specifiy access level for interface values.
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
struct GetOnlyTag : AccessTag {};
struct GetAndSetTag : AccessTag {};


} // namespace pex
