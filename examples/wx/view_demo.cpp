/**
  * @file view_demo.cpp
  *
  * @brief Demonstrates the usage of pex::wx::View and pex::wx::Button.
  *
  * @author Jive Helix (jivehelix@gmail.com)
  * @date 07 Aug 2020
  * @copyright Jive Helix
  * Licensed under the MIT license. See LICENSE file.
**/

#include <iostream>
#include <string>
#include <jive/formatter.h>
#include <tau/angles.h>
#include "pex/signal.h"
#include "pex/value.h"
#include "pex/converter.h"
#include "pex/wx/wxshim.h"
#include "pex/wx/view.h"
#include "pex/wx/button.h"


using Angle = pex::model::Value<double>;

// Create the control value without an observer.
using DegreesControl = pex::control::Value<void, Angle>;

using Signal = pex::model::Signal;
using ControlSignal = pex::control::Signal<void>;


/** Allow an control to use radians, while the model uses degrees. **/
struct RadiansFilter
{
    /** Convert to degrees on retrieval **/
    static double Get(double value)
    {
        return tau::ToRadians(value);
    }

    /** Convert back to radians on assignment **/
    static double Set(double value)
    {
        return tau::ToDegrees(value);
    }
};


using RadiansControl =
    pex::control::FilteredValue<void, Angle, RadiansFilter>;


class ExampleApp: public wxApp
{
public:
    ExampleApp()
        :
        angle_{42.0},
        signal_{},
        signalControl_{this->signal_}
    {

    }

    bool OnInit() override;

    void OnSignal_()
    {
        this->angle_.Set(this->angle_.Get() + 1.01);
    }

private:
    Angle angle_ = Angle(42.0);
    Signal signal_;
    pex::control::Signal<ExampleApp> signalControl_;
};


class ExampleFrame: public wxFrame
{
public:
    ExampleFrame(DegreesControl value, ControlSignal signal);
};


// Creates the main function for us, and initializes the app's run loop.
wxshimIMPLEMENT_APP(ExampleApp)


bool ExampleApp::OnInit()
{
    ExampleFrame *exampleFrame =
        new ExampleFrame(
            DegreesControl(this->angle_),
            ControlSignal(this->signal_));

    this->signalControl_.Connect(this, &ExampleApp::OnSignal_);
    exampleFrame->Show();
    return true;
}

/** Define other ways to format the angle **/
struct ThreeDigitsTraits: pex::DefaultConverterTraits
{
    static constexpr int precision = 3;
};


struct FifteenDigitsTraits: pex::DefaultConverterTraits
{
    static constexpr int precision = 15;
};


template<typename T>
using ThreeDigits = pex::Converter<T, ThreeDigitsTraits>;

template<typename T>
using FifteenDigits = pex::Converter<T, FifteenDigitsTraits>;


ExampleFrame::ExampleFrame(
    DegreesControl control,
    ControlSignal interfaceSignal)
    :
    wxFrame(nullptr, wxID_ANY, "pex::wx::View Demo")
{
    using Type = typename DegreesControl::Type;

    auto view =
        new pex::wx::View(this, control);

    auto three =
        new pex::wx::View<DegreesControl, ThreeDigits<Type>>(this, control);

    auto fifteen =
        new pex::wx::View<RadiansControl, FifteenDigits<Type>>(
            this,
            RadiansControl(control));

    auto button =
        new pex::wx::Button(this, "Press Me", interfaceSignal);

    auto topSizer = std::make_unique<wxBoxSizer>(wxVERTICAL);
    auto flags = wxLEFT | wxBOTTOM | wxRIGHT | wxEXPAND;

    topSizer->Add(view, 0, wxALL, 10);
    topSizer->Add(three, 0, flags, 10);
    topSizer->Add(fifteen, 0, flags, 10);
    topSizer->Add(button, 0, flags, 10);

    this->SetSizerAndFit(topSizer.release());
}
