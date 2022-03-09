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
#include "pex/wx/wxshim.h"
#include "pex/wx/view.h"
#include "pex/wx/field.h"
#include "pex/wx/labeled_widget.h"
#include "pex/value.h"
#include "tau/angles.h"


struct AngleFilter
{
    static constexpr auto minimum = -tau::Angles<double>::pi;
    static constexpr auto maximum = tau::Angles<double>::pi;

    static double Set(double input)
    {
        return std::min(maximum, std::max(minimum, input));
    }
};

using AngleRadians = pex::model::FilteredValue<double, AngleFilter>;
using RadiansControl = pex::control::Value<void, AngleRadians>;

/** Allow a control to use degrees, while the model uses radians. **/
struct DegreesFilter
{
    /** Convert to degrees on retrieval **/
    static double Get(double value)
    {
        return tau::ToDegrees(value);
    }

    /** Convert back to radians on assignment **/
    static double Set(double value)
    {
        return tau::ToRadians(value);
    }
};

using DegreesControl =
    pex::control::FilteredValue<void, AngleRadians, DegreesFilter>;


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
    ExampleFrame(RadiansControl value);
};


// Creates the main function for us, and initializes the app's run loop.
wxshimIMPLEMENT_APP(ExampleApp)


bool ExampleApp::OnInit()
{
    ExampleFrame *exampleFrame =
        new ExampleFrame(RadiansControl(this->angle_));

    exampleFrame->Show();
    return true;
}


ExampleFrame::ExampleFrame(RadiansControl control)
    :
    wxFrame(nullptr, wxID_ANY, "pex::wx::Field Demo")
{
    using namespace pex::wx;

    auto radiansView =
        LabeledWidget(
            this,
            WidgetMaker<View, RadiansControl>(control),
            "Radians:");

    auto degreesView =
        LabeledWidget(
            this,
            WidgetMaker<View, DegreesControl>(control),
            "Degrees:");

    auto radiansEntry =
        LabeledWidget(
            this,
            WidgetMaker<Field, RadiansControl>(control),
            "Radians:");

    auto degreesEntry =
        LabeledWidget(
            this,
            WidgetMaker<Field, DegreesControl>(control),
            "Degrees:");

    auto topSizer = std::make_unique<wxBoxSizer>(wxVERTICAL);
    auto flags = wxLEFT | wxBOTTOM | wxRIGHT | wxEXPAND;

    topSizer->Add(radiansView.Layout().release(), 0, wxALL, 10);
    topSizer->Add(degreesView.Layout().release(), 0, flags, 10);
    topSizer->Add(radiansEntry.Layout().release(), 0, flags, 10);
    topSizer->Add(degreesEntry.Layout().release(), 0, flags, 10);

    this->SetSizerAndFit(topSizer.release());
}
