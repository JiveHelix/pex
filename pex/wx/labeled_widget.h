/**
  * @file labeled_widget.h
  *
  * @brief Attach a label to any pex widget.
  *
  * @author Jive Helix (jivehelix@gmail.com)
  * @date 09 Aug 2020
  * @copyright Jive Helix
  * Licensed under the MIT license. See LICENSE file.
**/

#pragma once

#include "wxshim.h"
#include "pex/value.h"

namespace pex
{

namespace wx
{

/**
 ** Combines a label and a pex widget.
 ** Must define Type and Model.
 **/
template<typename Widget>
class LabeledWidget: public wxControl
{
public:
    template<typename CompatibleValue>
    LabeledWidget(
        wxWindow *parent,
        CompatibleValue value,
        const std::string &label,
        int layoutStyle = wxHORIZONTAL,
        long fieldStyle = 0)
        :
        wxControl(parent, wxID_ANY)
    {
        auto labelView = new wxStaticText(this, wxID_ANY, label);
        auto field = new Widget(this, value, fieldStyle);
        auto sizer = new wxBoxSizer(layoutStyle);

        auto flag = (layoutStyle == wxHORIZONTAL)
            ? wxRIGHT
            : wxBOTTOM | wxEXPAND;

        sizer->Add(labelView, 0, flag, 5);
        sizer->Add(field, 1, flag);
        this->SetSizerAndFit(sizer);
    }
};

} // wx

} // pex
