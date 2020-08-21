/**
  * @file button.h
  *
  * @brief A Button backed by a pex::interface::Value<bool>.
  *
  * @author Jive Helix (jivehelix@gmail.com)
  * @date 07 Aug 2020
  * @copyright Jive Helix
  * Licensed under the MIT license. See LICENSE file.
**/

#pragma once

#include "pex/wx/window.h"
#include "pex/signal.h"

namespace pex
{

namespace wx
{

class Button: public Window<wxButton>
{
public:
    using Base = Window<wxButton>;

    Button(
        wxWindow *parent,
        const std::string &label,
        pex::interface::Signal<void> signal,
        const WindowProperties &properties = WindowProperties{})
        :
        Base(
            parent,
            wxID_ANY,
            label,
            properties.position,
            properties.size,
            properties.style,
            wxDefaultValidator,
            properties.name),
        signal_(signal)
    {
        this->Bind(wxEVT_BUTTON, &Button::OnButton_, this);
    }

    void OnButton_(wxCommandEvent &)
    {
        this->signal_.Trigger();
    }

private:
    pex::interface::Signal<Button> signal_;
};


} // namespace wx

} // namespace pex
