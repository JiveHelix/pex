/**
  * @file tile.h
  * 
  * @brief Tile windows to fill the screen.
  * 
  * @author Jive Helix (jivehelix@gmail.com)
  * @date 28 Mar 2022
  * @copyright Jive Helix
  * Licensed under the MIT license. See LICENSE file.
**/

#pragma once

#include <vector>
#include "pex/wx/wxshim.h"

WXSHIM_PUSH_IGNORES
#include <wx/display.h>
WXSHIM_POP_IGNORES

namespace pex
{


namespace wx
{


inline void ScaleWindow(wxWindow *window, int size, int orient)
{
    auto windowSize = window->GetSize();
    auto clientSize = window->GetClientSize();
    float scale = 1.0f;

    if (orient == wxHORIZONTAL)
    {
        // width will be set to size, and height will be scaled to match aspect
        // ratio.

        int fixed = windowSize.GetWidth() - clientSize.GetWidth();

        scale =
            static_cast<float>(size - fixed)
            / static_cast<float>(clientSize.GetWidth());
    }
    else if (orient == wxVERTICAL)
    {
        int fixed = windowSize.GetHeight() - clientSize.GetHeight();
        
        scale =
            static_cast<float>(size - fixed)
            / static_cast<float>(clientSize.GetHeight());

    }
    else
    {
        throw std::invalid_argument("Unknown orientation.");
    }

    clientSize *= scale;

    window->SetClientSize(clientSize);
}


inline void Tile(const std::vector<wxWindow *> windows, int orient)
{
    if (windows.empty())
    {
        std::cerr << "Warning: No windows passed to Tile(...)" << std::endl;
        return;
    }

    int displayIndex = wxDisplay::GetFromWindow(windows[0]);

    if (displayIndex == wxNOT_FOUND)
    {
        throw std::runtime_error("Window is not connected to a display.");
    }

    wxDisplay display(static_cast<unsigned int>(displayIndex));

    auto screen = display.GetClientArea();

    int windowSize;

    if (orient == wxHORIZONTAL)
    {
        windowSize = screen.GetWidth() / windows.size();
    }
    else
    {
        windowSize = screen.GetHeight() / windows.size();
    }

    auto nextPosition = screen.GetTopLeft();

    for (auto window: windows)
    {
        ScaleWindow(window, windowSize, orient);
        window->SetPosition(nextPosition);

        if (orient == wxHORIZONTAL)
        {
            nextPosition += wxPoint(windowSize, 0);
        }
        else
        {
            nextPosition += wxPoint(0, windowSize);
        }
    }
}


} // end namespace wx


} // end namespace pex
