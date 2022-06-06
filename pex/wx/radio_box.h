/**
  * @file radio_box.h
  *
  * @brief A wx.RadioBox connected to a pex.Value control node.
  *
  * @author Jive Helix (jivehelix@gmail.com)
  * @date 11 Aug 2020
  * @copyright Jive Helix
  * Licensed under the MIT license. See LICENSE file.
**/

#pragma once

#include <vector>
#include <algorithm>

#include "pex/wx/wxshim.h"
#include "pex/value.h"
#include "pex/converter.h"
#include "pex/find_index.h"
#include "pex/wx/array_string.h"
#include "pex/wx/wx_chooser.h"


namespace pex
{

namespace wx
{


template
<   typename Chooser_,
    typename Convert = Converter<typename Chooser_::Value::Type>
>
class RadioBox: public wxRadioBox
{
public:
    using Chooser = control::ChangeObserver<RadioBox, Chooser_>;

    static_assert(
        !Chooser::choicesMayChange,
        "RadioBox choices cannot change after creation");

    using Type = typename Chooser::Type;
    using Selection = typename Chooser::Selection;
    using Choices = typename Chooser::Choices;
    using WxAdapter = WxChooser<typename Chooser::Type, Convert>;

    RadioBox(
        wxWindow *parent,
        Chooser_ chooser,
        const std::string &label = "",
        long style = wxRA_SPECIFY_ROWS)
        :
        wxRadioBox(
            parent,
            wxID_ANY,
            label,
            wxDefaultPosition,
            wxDefaultSize,
            WxAdapter::GetChoicesAsStrings(chooser.choices.Get()),
            0,
            style),
        chooser_(chooser)
    {
        assert(
            this->chooser_.selection.Get() <= std::numeric_limits<int>::max());

        this->SetSelection(static_cast<int>(this->chooser_.selection.Get()));

        PEX_LOG("Connect");
        this->chooser_.selection.Connect(this, &RadioBox::OnSelection_);

        this->Bind(wxEVT_RADIOBOX, &RadioBox::OnRadioBox_, this);
    }

    void OnSelection_(size_t index)
    {
        assert(index <= std::numeric_limits<int>::max());
        this->SetSelection(static_cast<int>(index));
    }

    void OnRadioBox_(wxCommandEvent &event)
    {
        auto index = event.GetSelection();

        if (index < 0)
        {
            return;
        }

        this->chooser_.selection.Set(static_cast<size_t>(index));
    }

private:
    Chooser chooser_;
};


} // namespace wx

} // namespace pex
