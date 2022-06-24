/**
  * @file pex_view.h
  *
  * @brief A read-only view of a pex.Value control node.
  *
  * @author Jive Helix (jivehelix@gmail.com)
  * @date 06 Aug 2020
  * @copyright Jive Helix
  * Licensed under the MIT license. See LICENSE file.
**/

#pragma once

#include <type_traits>
#include <string>
#include "pex/wx/wxshim.h"
#include "pex/value.h"
#include "pex/converter.h"
#include "pex/detail/argument.h"


namespace pex
{

namespace wx
{


template
<
    typename Control,
    typename Convert = Converter<typename Control::Type>
>
class View: public wxStaticText
{
public:
    using Base = wxStaticText;
    using Type = typename Control::Type;

    View(
        wxWindow *parent,
        Control value,
        long style = 0)
        :
        Base(
            parent,
            wxID_ANY,
            Convert::ToString(value.Get()),
            wxDefaultPosition,
            wxDefaultSize,
            style),
        value_(this, value)
    {
        PEX_LOG("Connect");
        this->value_.Connect(&View::OnValueChanged_);
    }

private:
    void OnValueChanged_(Argument<Type> value)
    {
        this->SetLabel(Convert::ToString(value));
        this->GetParent()->Layout();
    }

    using Value_ = pex::Terminus<View, Control>;
    Value_ value_;
};


template
<
    typename Value,
    typename Convert = Converter<typename Value::Type>
>
View<Value, Convert> * MakeView(wxWindow *parent, Value value, long style = 0)
{
    return new View<Value, Convert>(parent, value, style);
}


} // namespace wx

} // namespace pex
