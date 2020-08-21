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
#include "pex/wx/window.h"
#include "pex/value.h"


namespace pex
{

namespace wx
{


template<typename Value>
class CheckBox: public Window<wxCheckBox>
{
public:
    using Base = Window<wxCheckBox>;
    using Type = typename Value::Type;
    using Model = typename Value::Model;
    using Access = typename Value::Access;

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
        static_assert(
            std::is_same_v<Model, typename Compatible::Model>);

        static_assert(
            std::is_same_v<Access, typename Compatible::Access>);

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
    typename pex::interface::ObservedValue<Observer, Value>::Type value_;
};


} // namespace wx

} // namespace pex
