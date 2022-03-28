#pragma once

#include <vector>
#include <iostream>
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

    std::cout << "display: " << display.GetName() << std::endl;
    std::cout << screen << std::endl;

    int windowSize;

    if (orient == wxHORIZONTAL)
    {
        windowSize = screen.GetWidth() / windows.size();
    }
    else
    {
        windowSize = screen.GetHeight() / windows.size();
    }

    std::cout << "windowSize: " << windowSize << std::endl;

    auto nextPosition = screen.GetTopLeft();

    for (auto window: windows)
    {
        std::cout << "nextPosition: " << nextPosition << std::endl;

        ScaleWindow(window, windowSize, orient);
        window->SetPosition(nextPosition);

        std::cout << "Reported: " << window->GetRect() << std::endl;

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
