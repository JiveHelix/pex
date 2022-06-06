/**
  * @file button.h
  *
  * @brief A Button backed by a pex::control::Value<bool>.
  *
  * @author Jive Helix (jivehelix@gmail.com)
  * @date 07 Aug 2020
  * @copyright Jive Helix
  * Licensed under the MIT license. See LICENSE file.
**/

#pragma once

#include "pex/wx/ignores.h"

WXSHIM_PUSH_IGNORES
#include "wx/button.h"
WXSHIM_POP_IGNORES

#include "pex/signal.h"

namespace pex
{


namespace wx
{


struct ButtonOptions
{
    wxWindowID id = wxID_ANY;
    wxPoint position = wxDefaultPosition;
    wxSize size = wxDefaultSize;
    long style = 0;
};


class Button: public wxButton
{
public:
    using Base = wxButton;

    Button(
        wxWindow *parent,
        const std::string &label,
        pex::control::Signal<void> signal,
        const ButtonOptions &options = ButtonOptions{})
        :
        Base(
            parent,
            options.id,
            label,
            options.position,
            options.size,
            options.style),
        signal_(signal)
    {
        this->Bind(wxEVT_BUTTON, &Button::OnButton_, this);
    }

    Button(
        wxWindow *parent,
        const std::string &label,
        pex::control::Signal<void> signal,
        long style)
        :
        Base(parent, wxID_ANY, label, wxDefaultPosition, wxDefaultSize, style),
        signal_(signal)
    {
        this->Bind(wxEVT_BUTTON, &Button::OnButton_, this);
    }

    void OnButton_(wxCommandEvent &)
    {
        this->signal_.Trigger();
    }

private:
    pex::control::Signal<Button> signal_;
};


} // namespace wx

} // namespace pex
