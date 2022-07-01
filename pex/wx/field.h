/**
  * @file field.h
  *
  * @brief A field for textual or numeric entry, connected to
  * a pex::control::Value.
  *
  * @author Jive Helix (jivehelix@gmail.com)
  * @date 08 Aug 2020
  * @copyright Jive Helix
  * Licensed under the MIT license. See LICENSE file.
**/

#pragma once

#include "pex/wx/wxshim.h"
#include "pex/value.h"
#include "pex/converter.h"


namespace pex
{

namespace wx
{


template
<
    typename Control,
    typename ConverterTraits = DefaultConverterTraits
>
class Field: public wxTextCtrl
{
public:
    using Base = wxTextCtrl;
    using Type = typename Control::Type;
    using Convert = Converter<Type, ConverterTraits>;

    Field(
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
            style | wxTE_PROCESS_ENTER,
            wxDefaultValidator),
        value_{this, value},
        displayedString_{Convert::ToString(this->value_.Get())}
    {
        this->ChangeValue(this->displayedString_);
        this->Bind(wxEVT_TEXT_ENTER, &Field::OnEnter_, this);
        this->Bind(wxEVT_KILL_FOCUS, &Field::OnKillFocus_, this);

        PEX_LOG("Connect");
        this->value_.Connect(&Field::OnValueChanged_);
    }

private:
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
        auto userInput = this->GetValue().ToStdString();

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
            this->ChangeValue(this->displayedString_);
        }
        catch (std::invalid_argument &)
        {
            this->ChangeValue(this->displayedString_);
        }
    }

    void OnValueChanged_(Argument<Type> value)
    {
        this->displayedString_ = Convert::ToString(value);
        this->ChangeValue(this->displayedString_);
    }

    using Value = pex::Terminus<Field, Control>;

    Value value_;
    std::string displayedString_;
};


} // namespace wx


} // namespace pex
