/**
  * @file combo_box.h
  *
  * @brief A read-only wxComboBox backed by pex::Choices.
  *
  * @author Jive Helix (jivehelix@gmail.com)
  * @date 12 Aug 2020
  * @copyright Jive Helix
  * Licensed under the MIT license. See LICENSE file.
**/

#include "pex/wx/wxshim.h"
#include "pex/chooser.h"
#include "pex/wx/wx_chooser.h"


namespace pex
{

namespace wx
{


template
<
    typename T,
    typename Convert = Converter<T>
>
class ComboBox : public wxComboBox
{
public:
    using Base = wxComboBox;
    using Interface = typename ::pex::interface::Chooser<ComboBox, T>;
    using Selection = typename Interface::Selection;
    using Choices = typename Interface::Choices;
    using ChoicesVector = typename Choices::Type;
    using WxAdapter = WxChooser<T, Convert>;

    ComboBox(
        wxWindow *parent,
        ::pex::interface::Chooser<void, T> interface,
        long style = 0)
        :
        Base(
            parent,
            wxID_ANY,
            WxAdapter::GetSelectionAsString(
                interface.selection.Get(),
                interface.choices.Get()),
            wxDefaultPosition,
            wxDefaultSize,
            WxAdapter::GetChoicesAsStrings(interface.choices.Get()),
            style | wxCB_READONLY),
        selection_(interface.selection),
        choices_(interface.choices)
    {
        this->selection_.Connect(this, &ComboBox::OnSelectionChanged_);
        this->choices_.Connect(this, &ComboBox::OnChoicesChanged_);
        this->Bind(wxEVT_COMBOBOX, &ComboBox::OnComboBox_, this);
    }

    void OnSelectionChanged_(size_t index)
    {
        this->SetValue(
            WxAdapter::GetSelectionAsString(index, this->choices_.Get()));

        // Uncomment this line to see how the compiler enforces that choices_
        // is read-only.
        // this->choices_.Set(std::vector<std::string>{"foo", "bar"});
    }

    void OnChoicesChanged_(const ChoicesVector &choices)
    {
        this->Set(WxAdapter::GetChoicesAsStrings(choices));

        this->SetValue(
            WxAdapter::GetSelectionAsString(this->selection_.Get(), choices));
    }

    void OnComboBox_(wxCommandEvent &event)
    {
        auto index = event.GetSelection();

        if (index < 0)
        {
            return;
        }

        this->selection_.Set(static_cast<size_t>(index));
    }

    Selection selection_;
    Choices choices_;
};


} // namespace wx

} // namespace pex
