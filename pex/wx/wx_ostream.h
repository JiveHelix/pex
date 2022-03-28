/**
  * @file wx_ostream.h
  * 
  * @brief Format wxPoint, wxSize, and wxRect for logging.
  * 
  * @author Jive Helix (jivehelix@gmail.com)
  * @date 28 Mar 2022
  * @copyright Jive Helix
  * Licensed under the MIT license. See LICENSE file.
**/

#pragma once

#include <ostream>
#include "pex/wx/wxshim.h"


inline std::ostream & operator<<(
    std::ostream &outputStream,
    const wxPoint &point)
{
    return outputStream << "wxPoint(" << point.x << ", " << point.y << ")";
}


inline std::ostream & operator<<(
    std::ostream &outputStream,
    const wxSize &size)
{
    return outputStream << "wxSize(" << size.GetWidth() << ", "
        << size.GetHeight() << ")";
}


inline std::ostream & operator<<(
    std::ostream &outputStream,
    const wxRect &rect)
{
    return outputStream << "wxRect(" << rect.GetTopLeft() << ", "
        << rect.GetSize() << ")";
}



