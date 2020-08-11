/**
  * @file field.h
  *
  * @brief A field for textual or numeric entry, connected to
  * a pex::interface::Value.
  *
  * @author Jive Helix (jivehelix@gmail.com)
  * @date 08 Aug 2020
  * @copyright Jive Helix
  * Licensed under the MIT license. See LICENSE file.
**/

#pragma once

#include "wxshim.h"
#include "pex/value.h"
#include "pex/wx/pex_window.h"
#include "pex/wx/converter.h"


namespace pex
{

namespace wx
{


template
<
    typename Value,
    typename ConverterTraits = DefaultConverterTraits
>
class Field: public PexWindow<wxControl>
{
public:
    using Base = PexWindow<wxControl>;
    using Type = typename Value::Type;
    using Model = typename Value::Model;

    using Convert = Converter<Type, ConverterTraits>;

    template<typename AnyObserver, typename AnyFilter>
    Field(
        wxWindow *parent,
        pex::interface::Value_<AnyObserver, Model, AnyFilter> value,
        long style = 0)
        :
        Base(parent, wxID_ANY),
        value_{value},
        displayedString_{Convert::ToString(this->value_.Get())},
        textControl_{
            new wxTextCtrl(
                this,
                wxID_ANY,
                this->displayedString_,
                wxDefaultPosition,
                wxDefaultSize,
                style | wxTE_PROCESS_ENTER,
                wxDefaultValidator)}
    {
        this->textControl_->Bind(wxEVT_TEXT_ENTER, &Field::OnEnter_, this);
        this->textControl_->Bind(wxEVT_KILL_FOCUS, &Field::OnKillFocus_, this);
        this->value_.Connect(this, &Field::OnValueChanged_);
    }

    void OnEnter_(wxCommandEvent &)
    {
        this->ProcessUserInput_();
    }

    void OnKillFocus_(wxEvent &event)
    {
        this->ProcessUserInput_();
        event.Skip();
    }

    void ProcessUserInput_()
    {
        auto userInput = this->textControl_->GetValue().ToStdString();

        if (userInput == this->displayedString_)
        {
            // value has not changed.
            // Ignore this input.
            return;
        }

        try
        {
            this->value_.Set(Convert::ToValue(userInput));
        }
        catch (std::out_of_range &)
        {
            this->textControl_->ChangeValue(this->displayedString_);
        }
        catch (std::invalid_argument &)
        {
            this->textControl_->ChangeValue(this->displayedString_);
        }
    }

    void OnValueChanged_(typename ::pex::detail::Argument<Type>::Type value)
    {
        this->displayedString_ = Convert::ToString(value);
        this->textControl_->ChangeValue(this->displayedString_);
    }

    using Observer = Field<Value, ConverterTraits>;
    typename pex::interface::ObservedValue<Observer, Value>::Type value_;

    std::string displayedString_;
    wxTextCtrl *textControl_;
};


} // namespace wx


} // namespace pex
