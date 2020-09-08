/**
  * @file pex_view.h
  *
  * @brief A read-only view of a pex.Value interface node.
  *
  * @author Jive Helix (jivehelix@gmail.com)
  * @date 06 Aug 2020
  * @copyright Jive Helix
  * Licensed under the MIT license. See LICENSE file.
**/

#pragma once

#include <type_traits>
#include <string>
#include "wxshim.h"
#include "pex/value.h"
#include "pex/wx/window.h"
#include "pex/converter.h"
#include "pex/detail/argument.h"

namespace pex
{

namespace wx
{


template
<
    typename Value,
    typename Convert = Converter<typename Value::Type>
>
class View: public Window<wxStaticText>
{
public:
    using Base = Window<wxStaticText>;
    using Type = typename Value::Type;
    using Model = typename Value::Model;
    using Access = typename Value::Access;

    template<typename Compatible>
    View(
        wxWindow *parent,
        Compatible value,
        long style = 0)
        :
        Base(
            parent,
            wxID_ANY,
            Convert::ToString(Value(value).Get()),
            wxDefaultPosition,
            wxDefaultSize,
            style),
        value_(value)
    {
        static_assert(
            std::is_same_v<Model, typename Compatible::Model>);

        static_assert(
            std::is_same_v<Access, typename Compatible::Access>);

        this->value_.Connect(this, &View::OnValueChanged_);
    }

private:
    void OnValueChanged_(typename detail::Argument<Type>::Type value)
    {
        this->SetLabel(Convert::ToString(value));
        this->GetParent()->Layout();
    }

    using Observer = View<Value, Convert>;
    typename pex::interface::ObservedValue<Observer, Value>::Type value_;
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
