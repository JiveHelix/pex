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
    typename Control_,
    typename Convert = Converter<typename Control_::Type>
>
class ComboBox : public wxComboBox
{
public:
    using Base = wxComboBox;
    using This = ComboBox<Control_, Convert>;

    using Control = typename control::ChangeObserver<This, Control_>::Type;

    using Selection = typename Control::Selection;
    using Choices = typename Control::Choices;
    using ChoicesVector = typename Choices::Type;
    using WxAdapter = WxChooser<typename Control::Type, Convert>;

    ComboBox(
        wxWindow *parent,
        Control_ control,
        long style = 0)
        :
        Base(
            parent,
            wxID_ANY,
            WxAdapter::GetSelectionAsString(
                control.selection.Get(),
                control.choices.Get()),
            wxDefaultPosition,
            wxDefaultSize,
            WxAdapter::GetChoicesAsStrings(control.choices.Get()),
            style | wxCB_READONLY),
        selection_(control.selection),
        choices_(control.choices)
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
