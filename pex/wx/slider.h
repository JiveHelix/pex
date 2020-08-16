/**
  * @file slider.h
  *
  * @brief A wxSlider connected to a pex::Range.
  *
  * @author Jive Helix (jivehelix@gmail.com)
  * @date 14 Aug 2020
  * @copyright Jive Helix
  * Licensed under the MIT license. See LICENSE file.
**/

#pragma once

#include <limits>
#include <stdexcept>

#include "pex/range.h"
#include "pex/converter.h"

#include "wxshim.h"
#include "pex/wx/pex_window.h"
#include "pex/wx/view.h"

namespace pex
{

namespace wx
{


/** wxSlider uses `int`, so the default filter will attempt to convert T to
 ** `int`.
 **/

template<typename Target, typename T>
void RequireConvertible(T value)
{
    if (
        (value > std::numeric_limits<Target>::max())
        || (value < std::numeric_limits<Target>::min()))
    {
        throw std::range_error("value is not convertible to Target.");
    }
}

#ifndef NDEBUG
#define CHECK_TO_INT_RANGE(value) RequireConvertible<int>(value)
#define CHECK_FROM_INT_RANGE(T, value) RequireConvertible<T, int>(value)
#else
#define CHECK_TO_INT_RANGE(value)
#define CHECK_FROM_INT_RANGE(T, value)
#endif


template<typename T>
struct DefaultRangeFilter
{
    static int Get(T value)
    {
        CHECK_TO_INT_RANGE(value);
        return static_cast<int>(value);
    }

    static T Set(int value)
    {
        CHECK_FROM_INT_RANGE(T, value);
        return static_cast<T>(value);
    }
};


template
<
    typename RangeModel,
    typename Filter = DefaultRangeFilter<typename RangeModel::Type>
>
class Slider : public pex::wx::PexWindow<wxSlider>
{
public:
    using Base = PexWindow<wxSlider>;
    using This = Slider<RangeModel, Filter>;
    using Range = ::pex::interface::Range<This, RangeModel, Filter>;
    using Value = typename Range::Value;
    using Bound = typename Range::Bound;

    static_assert(std::is_same_v<int, typename Value::Type>);
    static_assert(std::is_same_v<int, typename Bound::Type>);

    template<typename CompatibleRange>
    Slider(
        wxWindow *parent,
        CompatibleRange range,
        long style = wxSL_HORIZONTAL)
        :
        Base(
            parent,
            wxID_ANY,
            Filter::Get(range.value.Get()),
            Filter::Get(range.minimum.Get()),
            Filter::Get(range.maximum.Get()),
            wxDefaultPosition,
            wxDefaultSize,
            style),
        value_(range.value),
        minimum_(range.minimum),
        maximum_(range.maximum),
        defaultValue_(this->value_.Get())
    {
        this->value_.Connect(this, &Slider::OnPexValue_);
        this->minimum_.Connect(this, &Slider::OnPexMinimum_);
        this->maximum_.Connect(this, &Slider::OnPexMaximum_);

        this->Bind(wxEVT_SLIDER, &Slider::OnSlider_, this);
        this->Bind(wxEVT_LEFT_DOWN, &Slider::OnSliderLeftDown_, this);
    }

    void OnPexValue_(int value)
    {
        this->SetValue(value);
    }

    void OnPexMinimum_(int minimum)
    {
        this->SetMin(minimum);
    }

    void OnPexMaximum_(int maximum)
    {
        this->SetMax(maximum);
    }

    void OnSlider_(wxCommandEvent &event)
    {
        this->value_.Set(event.GetInt());
    }

    void OnSliderLeftDown_(wxMouseEvent &event)
    {
        if (event.AltDown())
        {
            // Restore the default.
            this->value_.Set(this->defaultValue_);
        }
        else
        {
            event.Skip();
        }
    }

private:
    Value value_;
    Bound minimum_;
    Bound maximum_;
    int defaultValue_;
};


template
<
    typename RangeModel,
    typename Value,
    typename Filter = DefaultRangeFilter<typename RangeModel::Type>,
    typename Convert = pex::Converter<typename Value::Type>
>
class SliderAndValue : public pex::wx::PexWindow<wxControl>
{
public:
    using Base = pex::wx::PexWindow<wxControl>;

    template<typename CompatibleRange, typename CompatibleValue>
    SliderAndValue(
        wxWindow *parent,
        CompatibleRange range,
        CompatibleValue value,
        long style = wxSL_HORIZONTAL)
        :
        Base(parent, wxID_ANY)
    {
        auto slider = new Slider<RangeModel, Filter>(this, range, style);
        auto view = new View<Value, Convert>(this, value);

        auto sizerStyle =
            (wxSL_HORIZONTAL == style) ? wxHORIZONTAL : wxVERTICAL;

        auto sizer = new wxBoxSizer(sizerStyle);

        auto flag = (wxSL_HORIZONTAL == style)
            ? wxRIGHT | wxALIGN_CENTER_VERTICAL
            : wxBOTTOM | wxALIGN_CENTER;

        auto spacing = 5;

        sizer->Add(slider, 1, flag, spacing);
        sizer->Add(view, 0);
        this->SetSizerAndFit(sizer);
    }
};


} // namespace wx

} // namespace pex
