/**
  * @file pex_window.h
  *
  * @brief A base class that may turn out to be unnecessary.
  *
  * @author Jive Helix (jivehelix@gmail.com)
  * @date 06 Aug 2020
  * @copyright Jive Helix
  * Licensed under the MIT license. See LICENSE file.
**/

#pragma once

#include <string>
#include "wxshim.h"


namespace pex
{

namespace wx
{


struct WindowProperties
{
    std::string label = "";
    wxPoint position = wxDefaultPosition;
    wxSize size = wxDefaultSize;
    long style;
    std::string name = "";
};


/**
 ** @param WxBase The wx class to use as base class, which itself must derive
 ** from wxWindow.
 **/
template<typename WxBase>
class PexWindow: public WxBase
{
public:
    using WxBase::WxBase;

};


} // namespace wx

} // namespace pex


