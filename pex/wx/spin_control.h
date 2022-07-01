/**
  * @file spin_control.h
  * 
  * @brief A wxSpinCtrlDouble backed by a pex::Range for the value, minimum,
  * and maximum.
  * 
  * @author Jive Helix (jivehelix@gmail.com)
  * @date 17 Aug 2020
  * @copyright Jive Helix
  * Licensed under the MIT license. See LICENSE file.
**/

#pragma once

#include "pex/wx/ignores.h"

WXSHIM_PUSH_IGNORES
#include "wx/spinctrl.h"
WXSHIM_POP_IGNORES

#include "pex/range.h"
#include <jive/to_float.h>


namespace pex
{

namespace wx
{


template<typename RangeControl>
class SpinControl: public wxSpinCtrlDouble
{
public:
    using Base = wxSpinCtrlDouble;
    using This = SpinControl<RangeControl>;

    using Range = pex::control::ChangeObserver<This, RangeControl>;

    using Value = pex::Terminus<SpinControl, typename Range::Value>;
    using Limit = pex::Terminus<SpinControl, typename Range::Limit>;
    using Type = typename Value::Type;
    
    SpinControl(
        wxWindow *parent,
        RangeControl range,
        typename Value::Type increment,
        unsigned int digits = 4,
        long style = wxSP_ARROW_KEYS | wxTE_PROCESS_ENTER)
        :
        Base(
            parent,
            wxID_ANY,
            wxEmptyString,
            wxDefaultPosition,
            wxDefaultSize,
            style,
            static_cast<double>(range.minimum.Get()),
            static_cast<double>(range.maximum.Get()),
            static_cast<double>(range.value.Get()),
            static_cast<double>(increment)),
        value_(this, range.value),
        minimum_(this, range.minimum),
        maximum_(this, range.maximum)

    {
        this->SetDigits(digits);

        this->value_.Connect(&SpinControl::OnValue_);
        this->minimum_.Connect(&SpinControl::OnMinimum_);
        this->maximum_.Connect(&SpinControl::OnMaximum_);

        this->Bind(
            wxEVT_SPINCTRLDOUBLE,
            &SpinControl::OnSpinCtrlDouble_,
            this);

        this->Bind(
            wxEVT_TEXT_ENTER,
            &SpinControl::OnEnter_,
            this);
    }

    void OnValue_(Type value)
    {
        if (static_cast<double>(value) != this->GetValue())
        {
            this->SetValue(static_cast<double>(value));
        }
    }

    void OnMinimum_(Type minimum)
    {
        auto maximum = static_cast<double>(this->maximum_.Get());
        this->SetRange(static_cast<double>(minimum), maximum);
    }

    void OnMaximum_(Type maximum)
    {
        auto minimum = static_cast<double>(this->minimum_.Get());
        this->SetRange(minimum, static_cast<double>(maximum));
    }

    void OnSpinCtrlDouble_(wxSpinDoubleEvent &event)
    {
        this->value_.Set(static_cast<Type>(event.GetValue()));
        event.Skip();
    }

    void OnEnter_(wxCommandEvent &event)
    {
        // The documentation promises that wxEVT_SPINCTRLDOUBLE will be
        // generated when enter is pressed.
        // It is not.
        // Unfortunately the value of the spin ctrl is still the old value when
        // we intercept this wxCommandEvent from wTE_PROCESS_ENTER.
        // The comamnd event has the current value as a string, not
        // a double.
        this->value_.Set(
            static_cast<Type>(
                jive::ToFloat<double>(
                    static_cast<std::string>(event.GetString()))));

        event.Skip();
    }

private:
    Value value_;
    Limit minimum_;
    Limit maximum_;
};


} // namespace wx

} // namespace pex
