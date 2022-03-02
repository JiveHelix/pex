/**
  * @file check_box.h
  *
  * @brief A CheckBox backed by a pex::control::Value.
  *
  * @author Jive Helix (jivehelix@gmail.com)
  * @date 07 Aug 2020
  * @copyright Jive Helix
  * Licensed under the MIT license. See LICENSE file.
**/

#pragma once

#include "pex/wx/wxshim.h"
#include "pex/value.h"
#include "pex/is_compatible.h"


namespace pex
{

namespace wx
{


template<typename Value>
class CheckBox: public wxCheckBox
{
public:
    using Base = wxCheckBox;
    using Type = typename Value::Type;

    template<typename Compatible>
    CheckBox(
        wxWindow *parent,
        const std::string &label,
        Compatible value,
        long style = 0)
        :
        Base(
            parent,
            wxID_ANY,
            label,
            wxDefaultPosition,
            wxDefaultSize,
            style),
        value_(value)
    {
        static_assert(pex::IsCompatibleV<Value, Compatible>);

        this->SetValue(this->value_.Get());
        this->value_.Connect(this, &CheckBox::OnValueChanged_);
        this->Bind(wxEVT_CHECKBOX, &CheckBox::OnCheckBox_, this);
    }

    void OnValueChanged_(Type value)
    {
        this->SetValue(value);
    }

    void OnCheckBox_(wxCommandEvent &event)
    {
        this->value_.Set(event.IsChecked());
    }

private:
    using Observer = CheckBox<Value>;
    typename pex::control::ObservedValue<Observer, Value>::Type value_;
    size_t id_;
};


} // namespace wx

} // namespace pex
