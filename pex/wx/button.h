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

#include "pex/wx/ignores.h"

WXSHIM_PUSH_IGNORES
#include "wx/button.h"
WXSHIM_POP_IGNORES

#include "pex/signal.h"

namespace pex
{

namespace wx
{

class Button: public wxButton
{
public:
    using Base = wxButton;

    Button(
        wxWindow *parent,
        const std::string &label,
        pex::interface::Signal<void> signal)
        :
        Base(parent, wxID_ANY, label),
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
