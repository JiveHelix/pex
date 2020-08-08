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
#include "pex/detail/argument.h"

namespace pex
{

namespace wx
{


template<typename Value, typename = void>
struct DefaultFormatter
{
    using Type = typename Value::Type;

    static std::string ToString(typename detail::Argument<Type>::Type value)
    {
        return std::to_string(value);
    }
};


/** Specialization for std::string that doesn't create an extra copy. **/
template<typename Value>
struct DefaultFormatter
<
    Value,
    std::enable_if_t<std::is_same_v<typename Value::Type, std::string>>
>
{
    template<typename T>
    static T ToString(T &&value)
    {
        return std::forward<T>(value);
    }
};


template<typename Value, typename Formatter = DefaultFormatter<Value>>
class View: public PexWindow<wxStaticText>
{
public:
    using Base = PexWindow<wxStaticText>;
    using Type = typename Value::Type;
    
    template<typename AnyObserver, typename AnyFilter>
    View(
        wxWindow *parent,
        pex::interface::Value_<
            AnyObserver, typename Value::Model, AnyFilter> value,
        const WindowProperties &properties = WindowProperties{})
        :
        Base(
            parent,
            wxID_ANY,
            Formatter::ToString(Value(value).Get()),
            properties.position,
            properties.size,
            properties.style,
            properties.name),
        value_(value)
    {
        this->value_.Connect(this, &View::OnValueChanged_);
    }

private:
    void OnValueChanged_(typename detail::Argument<Type>::Type value)
    {
        this->SetLabel(Formatter::ToString(value));
        this->GetParent()->Layout();
    }
    
    using Observer = View<Value, Formatter>;
    typename pex::interface::ObservedValue<Observer, Value>::Type value_;
};


} // namespace wx

} // namespace pex
