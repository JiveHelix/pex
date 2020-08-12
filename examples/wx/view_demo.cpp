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
#include "wxshim.h"
#include "pex/wx/view.h"
#include "pex/wx/converter.h"
#include "pex/signal.h"
#include "pex/value.h"
#include "pex/wx/button.h"
#include "jive/formatter.h"
#include "jive/angles.h"


using Angle = pex::model::Value<double>;
using Interface = pex::interface::Value<void, Angle>;
using Signal = pex::model::Signal;
using InterfaceSignal = pex::interface::Signal<void>;


/** Allow an interface to use radians, while the model uses degrees. **/
struct DegreesFilter
{
    /** Convert to degrees on retrieval **/
    static double Get(double value)
    {
        return jive::ToRadians(value);
    }

    /** Convert back to radians on assignment **/
    static double Set(double value)
    {
        return jive::ToDegrees(value);
    }
};


using RadiansInterface =
    pex::interface::FilteredValue<void, Angle, DegreesFilter>;


class ExampleApp: public wxApp
{
public:
    ExampleApp()
        :
        angle_{42.0},
        signal_{},
        signalInterface_{&this->signal_}
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
    pex::interface::Signal<ExampleApp> signalInterface_;
};


class ExampleFrame: public wxFrame
{
public:
    ExampleFrame(Interface value, InterfaceSignal signal);
};

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"

// Creates the main function for us, and initializes the app's run loop.
wxIMPLEMENT_APP(ExampleApp);

#pragma GCC diagnostic pop


bool ExampleApp::OnInit()
{
    ExampleFrame *exampleFrame =
        new ExampleFrame(
            Interface(&this->angle_),
            InterfaceSignal(&this->signal_));

    this->signalInterface_.Connect(this, &ExampleApp::OnSignal_);
    exampleFrame->Show();
    return true;
}

/** Define other ways to format the angle **/
struct ThreeDigitsTraits: pex::wx::DefaultConverterTraits
{
    static constexpr int precision = 3;
};


struct FifteenDigitsTraits: pex::wx::DefaultConverterTraits
{
    static constexpr int precision = 15;
};


template<typename T>
using ThreeDigits = pex::wx::Converter<T, ThreeDigitsTraits>;

template<typename T>
using FifteenDigits = pex::wx::Converter<T, FifteenDigitsTraits>;


ExampleFrame::ExampleFrame(
    Interface interface,
    InterfaceSignal interfaceSignal)
    :
    wxFrame(nullptr, wxID_ANY, "pex::wx::View Demo")
{
    using Type = typename Interface::Type;

    auto view =
        new pex::wx::View<Interface>(this, interface);
    
    auto three =
        new pex::wx::View<Interface, ThreeDigits<Type>>(this, interface);

    auto fifteen =
        new pex::wx::View<RadiansInterface, FifteenDigits<Type>>(this, interface);

    auto button =
        new pex::wx::Button(this, "Press Me", interfaceSignal);

    wxBoxSizer *topSizer = new wxBoxSizer(wxVERTICAL); 
    auto flags = wxLEFT | wxBOTTOM | wxRIGHT | wxEXPAND;

    topSizer->Add(view, 0, wxALL, 10);
    topSizer->Add(three, 0, flags, 10);
    topSizer->Add(fifteen, 0, flags, 10);
    topSizer->Add(button, 0, flags, 10);

    this->SetSizerAndFit(topSizer);
}
