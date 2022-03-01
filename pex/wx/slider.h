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


/** wxSlider uses `int`, so the default filter will attempt to convert T to
 ** `int`.
 **/


template
<
    typename RangeInterface
>
class Slider : public wxSlider
{
public:
    using Base = wxSlider;
    using This = Slider<RangeInterface>;

    // Value and Bound are observed by This
    using Value = typename
        pex::interface::ObservedValue
        <
            This,
            typename RangeInterface::Value
        >::Type;

    using Bound = typename
        pex::interface::ObservedValue
        <
            This,
            typename RangeInterface::Bound
        >::Type;
    
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
            range.value.Get(),
            range.minimum.Get(),
            range.maximum.Get(),
            wxDefaultPosition,
            wxDefaultSize,
            style),
        value_(range.value),
        minimum_(range.minimum),
        maximum_(range.maximum),
        defaultValue_(this->value_.Get())
    {
        this->value_.Connect(this, &Slider::OnValue_);
        this->minimum_.Connect(this, &Slider::OnMinimum_);
        this->maximum_.Connect(this, &Slider::OnMaximum_);

        this->Bind(wxEVT_SLIDER, &Slider::OnSlider_, this);
        this->Bind(wxEVT_LEFT_DOWN, &Slider::OnSliderLeftDown_, this);
        
        // wxSlider appears to underreport its minimum size, which causes the
        // thumb to be clipped.
        auto bestSize = this->GetBestSize();
        auto bestHeight = bestSize.GetHeight();
        bestSize.SetHeight(static_cast<int>(bestHeight * 1.25));
        this->SetMinSize(bestSize);
    }

    void OnValue_(int value)
    {
        this->SetValue(value);
    }

    void OnMinimum_(int minimum)
    {
        this->SetMin(minimum);
    }

    void OnMaximum_(int maximum)
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


template<int base_, int width_, int precision_>
struct ViewTraits
{
    static constexpr int base = base_;
    static constexpr int width = width_;
    static constexpr int precision = precision_;
};


template
<
    typename RangeInterface,
    typename ValueInterface,
    int viewPrecision = 6,
    typename Convert =
        pex::Converter
        <
            typename ValueInterface::Type,
            ViewTraits<10, viewPrecision + 2, viewPrecision>
        >
>
class SliderAndValue : public wxControl
{
public:
    using Base = wxControl;

    // range is filtered to an int for direct use in the wx.Slider.
    // value is the value from the model for display in the view.
    template<typename CompatibleRange, typename CompatibleValue>
    SliderAndValue(
        wxWindow *parent,
        CompatibleRange range,
        CompatibleValue value,
        long style = wxSL_HORIZONTAL)
        :
        Base(parent, wxID_ANY)
    {
        // Create slider and view as children of the this wxWindow.
        // They are memory managed by the the wxWindow from their creation.
        auto slider = new Slider<RangeInterface>(this, range, style);
        auto view = new View<ValueInterface, Convert>(this, value);

        // Use a mono-spaced font for display so that the width of the view
        // remains constant as the value changes.
        view->SetFont(wxFont(wxFontInfo().Family(wxFONTFAMILY_MODERN)));

        auto sizerStyle =
            (wxSL_HORIZONTAL == style) ? wxHORIZONTAL : wxVERTICAL;

        auto sizer = std::make_unique<wxBoxSizer>(sizerStyle);

        auto flag = (wxSL_HORIZONTAL == style)
            ? wxRIGHT | wxALIGN_CENTER_VERTICAL | wxEXPAND
            : wxBOTTOM | wxALIGN_CENTER | wxEXPAND;

        auto spacing = 5;

        sizer->Add(slider, 1, flag, spacing);

        sizer->Add(
            view,
            0,
            (wxSL_HORIZONTAL == wxHORIZONTAL)
                ? wxALIGN_CENTER_VERTICAL
                : wxALIGN_CENTER);

        this->SetSizerAndFit(sizer.release());
    }
};


} // namespace wx

} // namespace pex
