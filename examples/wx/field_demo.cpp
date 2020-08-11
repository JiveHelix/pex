/**
  * @file field_demo.cpp
  * 
  * @brief Demonstrates the use of pex::wx::Field.
  * 
  * @author Jive Helix (jivehelix@gmail.com)
  * @date 09 Aug 2020
  * @copyright Jive Helix
  * Licensed under the MIT license. See LICENSE file.
**/

#include <iostream>
#include <string>
#include "wxshim.h"
#include "pex/wx/view.h"
#include "pex/wx/field.h"
#include "pex/wx/labeled_widget.h"
#include "pex/value.h"
#include "jive/angles.h"


struct AngleFilter
{
    static constexpr auto minimum = -jive::Angles<double>::pi;
    static constexpr auto maximum = jive::Angles<double>::pi;

    static double Set(double input)
    {
        return std::min(maximum, std::max(minimum, input));
    }
};

using AngleRadians = pex::model::FilteredValue<double, AngleFilter>;
using RadiansInterface = pex::interface::Value<void, AngleRadians>;

/** Allow an interface to use degrees, while the model uses radians. **/
struct DegreesFilter
{
    /** Convert to degrees on retrieval **/
    static double Get(double value)
    {
        return jive::ToDegrees(value);
    }

    /** Convert back to radians on assignment **/
    static double Set(double value)
    {
        return jive::ToRadians(value);
    }
};

using DegreesInterface =
    pex::interface::FilteredValue<void, AngleRadians, DegreesFilter>;


class ExampleApp: public wxApp
{
public:
    ExampleApp()
        :
        angle_{0.0}
    {
        this->angle_.Connect(this, &ExampleApp::OnUpdate_); 
    }

    bool OnInit() override;

private:
    static void OnUpdate_(void *, double value)
    {
        std::cout << "angle updated: " << value << std::endl;
    }

private:
    AngleRadians angle_;
};


class ExampleFrame: public wxFrame
{
public:
    ExampleFrame(RadiansInterface value);
};

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"

// Creates the main function for us, and initializes the app's run loop.
wxIMPLEMENT_APP(ExampleApp);

#pragma GCC diagnostic pop


bool ExampleApp::OnInit()
{
    ExampleFrame *exampleFrame =
        new ExampleFrame(RadiansInterface(&this->angle_));

    exampleFrame->Show();
    return true;
}


ExampleFrame::ExampleFrame(RadiansInterface interface)
    :
    wxFrame(nullptr, wxID_ANY, "pex::wx::Field Demo")
{
    auto radiansView =
        new pex::wx::LabeledWidget<pex::wx::View<RadiansInterface>>(
            this,
            interface,
            "Radians:");

    auto degreesView =
        new pex::wx::LabeledWidget<pex::wx::View<DegreesInterface>>(
            this,
            interface,
            "Degrees:");

    auto radiansEntry = 
        new pex::wx::LabeledWidget<pex::wx::Field<RadiansInterface>>(
            this,
            interface,
            "Radians:");

    auto degreesEntry = 
        new pex::wx::LabeledWidget<pex::wx::Field<DegreesInterface>>(
            this,
            interface,
            "Degrees:");

    wxBoxSizer *topSizer = new wxBoxSizer(wxVERTICAL); 
    auto flags = wxLEFT | wxBOTTOM | wxRIGHT | wxEXPAND;

    topSizer->Add(radiansView, 0, wxALL, 10);
    topSizer->Add(degreesView, 0, flags, 10);
    topSizer->Add(radiansEntry, 0, flags, 10);
    topSizer->Add(degreesEntry, 0, flags, 10);

    this->SetSizerAndFit(topSizer);
}
