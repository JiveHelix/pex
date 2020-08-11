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
#include "pex/wx/pex_window.h"
#include "pex/wx/converter.h"
#include "pex/detail/argument.h"

namespace pex
{

namespace wx
{


template
<
    typename Value,
    typename ConverterTraits = DefaultConverterTraits
>
class View: public PexWindow<wxStaticText>
{
public:
    using Base = PexWindow<wxStaticText>;
    using Type = typename Value::Type;
    using Model = typename Value::Model;
    using Convert = Converter<Type, ConverterTraits>;

    template<typename AnyObserver, typename AnyFilter>
    View(
        wxWindow *parent,
        pex::interface::Value_<AnyObserver, Model, AnyFilter> value,
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
        this->value_.Connect(this, &View::OnValueChanged_);
    }

private:
    void OnValueChanged_(typename detail::Argument<Type>::Type value)
    {
        this->SetLabel(Convert::ToString(value));
        this->GetParent()->Layout();
    }

    using Observer = View<Value, ConverterTraits>;
    typename pex::interface::ObservedValue<Observer, Value>::Type value_;
};


} // namespace wx

} // namespace pex
