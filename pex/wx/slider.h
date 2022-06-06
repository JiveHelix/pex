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
#include <cstdint>

#include "pex/range.h"
#include "pex/converter.h"

#include "pex/wx/wxshim.h"
#include "pex/wx/view.h"


namespace pex
{

namespace wx
{


namespace detail
{

struct StyleFilter
{
public:
    StyleFilter(long style, int maximum)
        :
        isVertical_(style & wxSL_VERTICAL),
        maximum_(maximum)
    {

    }

    int operator()(int value)
    {
        if (!this->isVertical_)
        {
            return value;
        }
        
        return this->maximum_ - value;
    }

    void SetMaximum(int maximum)
    {
        this->maximum_ = maximum;
    }

private:
    bool isVertical_;
    int maximum_;
};


} // end namespace detail


/** wxSlider uses `int`, so the default filter will attempt to convert T to
 ** `int`.
 **/

template
<
    typename RangeControl
>
class Slider : public wxSlider
{
public:
    using Base = wxSlider;
    using This = Slider<RangeControl>;

    // Value and Limit are observed by This
    using Range = pex::control::ChangeObserver<This, RangeControl>;

    using Value = typename Range::Value;
    using Limit = typename Range::Limit;
    
    static_assert(
        std::is_same_v<int, typename Value::Type>,
        "Slider control uses int");

    static_assert(
        std::is_same_v<int, typename Limit::Type>,
        "Slider control uses int");

    Slider(
        wxWindow *parent,
        RangeControl range,
        long style = wxSL_HORIZONTAL)
        :
        Base(
            parent,
            wxID_ANY,
            detail::StyleFilter(
                style,
                range.maximum.Get())(range.value.Get()),
            range.minimum.Get(),
            range.maximum.Get(),
            wxDefaultPosition,
            wxDefaultSize,
            style),
        value_(range.value),
        minimum_(range.minimum),
        maximum_(range.maximum),
        defaultValue_(this->value_.Get()),
        styleFilter_(style, range.maximum.Get())
    {
        PEX_LOG("Connect");
        this->value_.Connect(this, &Slider::OnValue_);

        PEX_LOG("Connect");
        this->minimum_.Connect(this, &Slider::OnMinimum_);

        PEX_LOG("Connect");
        this->maximum_.Connect(this, &Slider::OnMaximum_);

        this->Bind(wxEVT_SLIDER, &Slider::OnSlider_, this);
        this->Bind(wxEVT_LEFT_DOWN, &Slider::OnSliderLeftDown_, this);
        
        // wxSlider appears to underreport its minimum size, which causes the
        // thumb to be clipped.
        // TODO: Determine whether this affects platforms other than wxMac.
        auto bestSize = this->GetBestSize();
        auto bestHeight = bestSize.GetHeight();
        bestSize.SetHeight(static_cast<int>(bestHeight * 1.25));
        this->SetMinSize(bestSize);
    }

    void OnValue_(int value)
    {
        this->SetValue(this->styleFilter_(value));
    }

    void OnMinimum_(int minimum)
    {
        if (this->defaultValue_ < minimum)
        {
            this->defaultValue_ = minimum;
        }

        this->SetMin(minimum);
    }

    void OnMaximum_(int maximum)
    {
        if (this->defaultValue_ > maximum)
        {
            this->defaultValue_ = maximum;
        }

        this->SetMax(maximum);
        this->styleFilter_.SetMaximum(maximum);
    }

    void OnSlider_(wxCommandEvent &event)
    {
        // wx generates multiple wxEVT_SLIDER events with the same value.
        // We will only send changes.
        auto newValue = this->styleFilter_(event.GetInt());
        
        if (newValue != this->value_.Get())
        {
            this->value_.Set(newValue);
        }
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
    Limit minimum_;
    Limit maximum_;
    int defaultValue_;
    detail::StyleFilter styleFilter_;
};


template<int base_, int width_, int precision_>
struct ViewTraits
{
    static constexpr int base = base_;
    static constexpr int width = width_;
    static constexpr int precision = precision_;
};


template
<
    typename RangeControl,
    typename ValueControl,
    int viewPrecision = 6,
    typename Convert =
        pex::Converter
        <
            typename ValueControl::Type,
            ViewTraits<10, viewPrecision + 2, viewPrecision>
        >
>
class SliderAndValue : public wxControl
{
public:
    using Base = wxControl;

    // range is filtered to an int for direct use in the wx.Slider.
    // value is the value from the model for display in the view.
    SliderAndValue(
        wxWindow *parent,
        RangeControl range,
        ValueControl value,
        long style = wxSL_HORIZONTAL)
        :
        Base(parent, wxID_ANY)
    {
        // Create slider and view as children of the this wxWindow.
        // They are memory managed by the the wxWindow from their creation.
        auto slider = new Slider<RangeControl>(this, range, style);
        auto view = new View<ValueControl, Convert>(this, value);

        // Use a mono-spaced font for display so that the width of the view
        // remains constant as the value changes.
        view->SetFont(wxFont(wxFontInfo().Family(wxFONTFAMILY_MODERN)));

        auto sizerStyle =
            (wxSL_HORIZONTAL == style) ? wxHORIZONTAL : wxVERTICAL;

        auto sizer = std::make_unique<wxBoxSizer>(sizerStyle);

        auto flag = (wxSL_HORIZONTAL == style)
            ? wxRIGHT | wxEXPAND
            : wxBOTTOM | wxEXPAND;

        auto spacing = 5;

        sizer->Add(slider, 1, flag, spacing);

        sizer->Add(
            view,
            0,
            (wxSL_HORIZONTAL == wxHORIZONTAL)
                ? wxALIGN_CENTER
                : wxALIGN_CENTER_VERTICAL);

        this->SetSizerAndFit(sizer.release());
    }
};


} // namespace wx

} // namespace pex
