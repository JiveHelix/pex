/**
  * @file spin_control_double.h
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

namespace pex
{

namespace wx
{


template<typename RangeModel>
class SpinControlDouble : public wxSpinCtrlDouble
{
public:
    using Base = wxSpinCtrlDouble;
    using This = SpinControlDouble<RangeModel>;
    using Range = ::pex::control::Range<This, RangeModel>;
    using Value = typename Range::Value;
    using Bound = typename Range::Bound;
    using Type = typename Value::Type;
    
    template<typename CompatibleRange>
    SpinControlDouble(
        wxWindow *parent,
        CompatibleRange range,
        typename Value::Type increment,
        unsigned int digits = 4,
        long style = wxSP_ARROW_KEYS)
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
        value_(range.value),
        minimum_(range.minimum),
        maximum_(range.maximum)

    {
        this->SetDigits(digits);
        this->value_.Connect(this, &SpinControlDouble::OnValue_);
        this->minimum_.Connect(this, &SpinControlDouble::OnMinimum_);
        this->maximum_.Connect(this, &SpinControlDouble::OnMaximum_);

        this->Bind(
            wxEVT_SPINCTRLDOUBLE,
            &SpinControlDouble::OnSpinCtrlDouble_,
            this);
    }

    void OnValue_(Type value)
    {
        this->SetValue(static_cast<double>(value));
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
    }

private:
    Value value_;
    Bound minimum_;
    Bound maximum_;
};


} // namespace wx

} // namespace pex
