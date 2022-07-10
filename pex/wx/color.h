#pragma once


#include <tau/color.h>
#include "pex/pex.h"
#include "pex/wx/slider.h"
#include "pex/wx/labeled_widget.h"


namespace pex
{


namespace wx
{


using HsvGroup = pex::Group
    <
        tau::HsvFields,
        tau::HsvTemplate<float>::template Template,
        tau::Hsv<float>
    >;

using Hsv = typename HsvGroup::Plain;
using HsvModel = typename HsvGroup::Model;
using HsvControl = typename HsvGroup::Control<void>;

template<typename Observer>
using HsvTerminus = typename HsvGroup::Terminus<Observer>;


using HsvRangeGroup = pex::RangeGroup
    <
        tau::HsvFields,
        tau::HsvTemplate<float>::template Template,
        HsvControl
    >;

using HsvRanges = HsvRangeGroup::Models;
using HsvRangesControl = HsvRangeGroup::Controls;


class ColorPreview: public wxPanel
{
public:
    ColorPreview(
        wxWindow *parent,
        const tau::Rgb<uint8_t> &color,
        wxSize size = wxSize(65, 65))
        :
        wxPanel(parent, wxID_ANY, wxDefaultPosition, size)
    {
        this->SetColor(color);
    }

    void SetColor(const tau::Rgb<uint8_t> &color)
    {
        this->SetBackgroundColour(wxColour(color.red, color.green, color.blue));
        this->Refresh();
    }
};


using HueRange =
    pex::control::LinearRange
    <
        void,
        decltype(HsvRangesControl::hue),
        int,
        10,
        0
    >;

using SaturationRange =
    pex::control::LinearRange

    <
        void,
        decltype(HsvRangesControl::saturation),
        int,
        1000,
        0
    >;

using ValueRange =
    pex::control::LinearRange
    <
        void,
        decltype(HsvRangesControl::value),
        int,
        1000,
        0
    >;


class HsvPicker: public wxControl
{
public:

    HsvPicker(
        wxWindow *parent,
        HsvControl control)
        :
        wxControl(parent, wxID_ANY),
        terminus_(this, control),
        hsvRanges_(control)
    {
        PEX_LOG("\n\n ********* picker ctor ************* \n\n");

        this->hsvRanges_.hue.SetLimits(0.0f, 360.0f);
        PEX_LOG("picker ctor");
        this->hsvRanges_.saturation.SetLimits(0.0f, 1.0f);
        PEX_LOG("picker ctor");
        this->hsvRanges_.value.SetLimits(0.0f, 1.0f);
        PEX_LOG("picker ctor");

        this->terminus_.Connect(&HsvPicker::OnColorChanged_);
        PEX_LOG("picker ctor");

        HsvRangesControl rangesControl(this->hsvRanges_);
        PEX_LOG("picker ctor");

        using HueSlider =
            SliderAndValue<HueRange, decltype(control.hue), 5>;

        PEX_LOG("picker ctor");
        auto hue = pex::wx::LabeledWidget(
            this,
            "Hue",
            new HueSlider(
                this,
                HueRange(rangesControl.hue),
                control.hue));

        PEX_LOG("picker ctor");
        using SaturationSlider =
            SliderAndValue<SaturationRange, decltype(control.saturation), 4>;

        PEX_LOG("picker ctor");
        auto saturation = pex::wx::LabeledWidget(
            this,
            "Saturation",
            new SaturationSlider(
                this,
                SaturationRange(rangesControl.saturation),
                control.saturation));

        PEX_LOG("picker ctor");
        using ValueSlider =
            SliderAndValue<ValueRange, decltype(control.value), 4>;

        PEX_LOG("\n\n picker ctor \n\n");
        auto value = pex::wx::LabeledWidget(
            this,
            "Value",
            new ValueSlider(
                this,
                ValueRange(rangesControl.value),
                control.value));
        ///**
        PEX_LOG("\n\n picker ctor \n\n");
        this->colorPreview_ = new ColorPreview(
            this,
            tau::HsvToRgb<uint8_t>(control.Get()));

        auto sizer = std::make_unique<wxBoxSizer>(wxHORIZONTAL);
        
        auto sliderLayout = 
            LayoutLabeled(LayoutOptions{}, hue, saturation, value);

        sizer->Add(sliderLayout.release(), 1, wxRight, 5);

        auto vertical = std::make_unique<wxBoxSizer>(wxVERTICAL);

        vertical->Add(
            this->colorPreview_,
            0,
            wxALIGN_CENTER | wxALL,
            10);

        sizer->Add(vertical.release(), 0, wxEXPAND);

        this->SetSizerAndFit(sizer.release());
        PEX_LOG("\n\n picker ctor \n\n");
    }

    virtual ~HsvPicker()
    {
        this->DestroyChildren();
    }

private:
    void OnColorChanged_(const Hsv &hsv)
    {
        this->colorPreview_->SetColor(tau::HsvToRgb<uint8_t>(hsv));
    }

private:
    HsvTerminus<HsvPicker> terminus_;
    HsvRanges hsvRanges_;
    ColorPreview * colorPreview_;
};


} // end namespace wx


} // end namespace pex
