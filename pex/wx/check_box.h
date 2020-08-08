/**
  * @file check_box.h
  *
  * @brief A CheckBox backed by a pex::interface::Value.
  *
  * @author Jive Helix (jivehelix@gmail.com)
  * @date 07 Aug 2020
  * @copyright Jive Helix
  * Licensed under the MIT license. See LICENSE file.
**/

#pragma once

#include "wxshim.h"
#include "pex/wx/pex_window.h"
#include "pex/value.h"

namespace pex
{

namespace wx
{


template<typename Value>
class CheckBox: public PexWindow<wxCheckBox>
{
public:
    using Base = PexWindow<wxCheckBox>;

    template<typename AnyObserver, typename AnyFilter>
    CheckBox(
        wxWindow *parent,
        const std::string &label,
        pex::interface::Value_<
            AnyObserver, typename Value::Model, AnyFilter> value,
        const WindowProperties &properties = WindowProperties{})
        :
        Base(
            parent,
            wxID_ANY,
            label,
            properties.position,
            properties.size,
            properties.style),
        value_(value)
    {
        this->RegisterTubes(this->value_);
        this->SetValue(this->value_.Get());
        this->value_.Connect(this, &CheckBox::OnValueChanged_);
        this->Bind(wxEVT_CHECKBOX, &CheckBox::OnCheckBox_, this);
    }

    void OnValueChanged_(bool value)
    {
        this->SetValue(value);
    }

    void OnCheckBox_(wxCommandEvent &event)
    {
        this->value_.Set(event.IsChecked());
    }

private:
    using Observer = CheckBox<Value>;
    typename pex::interface::ObservedValue<Observer, Value>::Type value_;
};


} // namespace wx

} // namespace pex
