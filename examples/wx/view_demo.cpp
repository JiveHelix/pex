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
#include "wxshim.h"
#include "pex/wx/view.h"
#include <iostream>
#include <string>
#include <thread>

#include "pex/signal.h"
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
        this->angle_.Set(this->angle_.Get() + 1.0);
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

private:
    ~ExampleFrame()
    {
        std::cout << "~ExampleFrame()" << std::endl;
    }

    bool Destroy() override
    {
        std::cout << "ExampleFrame::Destroy()" << std::endl;
        return wxFrame::Destroy();
    }
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
struct ThreeDigits
{
    static std::string ToString(Angle::Type value)
    {
        return jive::Formatter<32>("%.3f", value); 
    }
};


struct FifteenDigits
{
    static std::string ToString(Angle::Type value)
    {
        return jive::Formatter<32>("%.15f", value); 
    }
};


ExampleFrame::ExampleFrame(
    Interface interface,
    InterfaceSignal interfaceSignal)
    :
    wxFrame(nullptr, wxID_ANY, "pex::wx::View Demo")
{
    auto view =
        new pex::wx::View<Interface>(this, interface);
    
    auto three =
        new pex::wx::View<Interface, ThreeDigits>(this, interface);

    auto fifteen =
        new pex::wx::View<RadiansInterface, FifteenDigits>(this, interface);

    auto button =
        new pex::wx::Button(this, "Press Me", interfaceSignal);

    wxBoxSizer *topSizer = new wxBoxSizer(wxVERTICAL); 
    auto flags = wxLEFT | wxBOTTOM | wxRIGHT;

    topSizer->Add(view, 0, wxALL, 10);
    topSizer->Add(three, 0, flags, 10);
    topSizer->Add(fifteen, 0, flags, 10);
    topSizer->Add(button, 0, flags, 10);

    this->SetSizerAndFit(topSizer);
}
