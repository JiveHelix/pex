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

using Message = pex::model::Value<std::string>;
using MessageControl = pex::control::Value<void, Message>;

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
        this->message_.Set("This is the initial message");
    }

    bool OnInit() override;

private:
    static void OnUpdate_(void * context, double value)
    {
        auto self = static_cast<ExampleApp *>(context);

        self->message_.Set(
            "The angle has been updated to: " + std::to_string(value));
    }

private:
    AngleRadians angle_;
    Message message_;
};


class ExampleFrame: public wxFrame
{
public:
    ExampleFrame(
        RadiansControl radians,
        DegreesControl degrees,
        MessageControl message);
};


// Creates the main function for us, and initializes the app's run loop.
wxshimIMPLEMENT_APP(ExampleApp)


bool ExampleApp::OnInit()
{
    ExampleFrame *exampleFrame =
        new ExampleFrame(
            RadiansControl(this->angle_),
            DegreesControl(this->angle_),
            MessageControl(this->message_));

    exampleFrame->Show();
    return true;
}


ExampleFrame::ExampleFrame(
    RadiansControl radians,
    DegreesControl degrees,
    MessageControl message)
    :
    wxFrame(nullptr, wxID_ANY, "pex::wx::Field Demo")
{
    using namespace pex::wx;

    auto radiansView =
        LabeledWidget(
            this,
            "Radians:",
            new View(this, radians));

    auto degreesView =
        LabeledWidget(
            this,
            "Degrees:",
            new View(this, degrees));

    auto radiansEntry =
        LabeledWidget(
            this,
            "Radians:",
            new Field(this, radians));

    auto degreesEntry =
        LabeledWidget(
            this,
            "Degrees:",
            new Field(this, degrees));

    auto messageField =
        LabeledWidget(
            this,
            "Message:",
            new Field(this, message));

    auto topSizer = std::make_unique<wxBoxSizer>(wxVERTICAL);

#if 1
    auto sizer = LayoutLabeled(
        LayoutOptions{},
        radiansView,
        degreesView,
        radiansEntry,
        degreesEntry,
        messageField);

    topSizer->Add(sizer.release(), 1, wxALL | wxEXPAND, 10);
#else
    auto flags = wxLEFT | wxBOTTOM | wxRIGHT | wxEXPAND;

    topSizer->Add(radiansView.Layout().release(), 0, wxALL, 10);
    topSizer->Add(degreesView.Layout().release(), 0, flags, 10);
    topSizer->Add(radiansEntry.Layout().release(), 0, flags, 10);
    topSizer->Add(degreesEntry.Layout().release(), 0, flags, 10);
    topSizer->Add(messageField.Layout().release(), 0, flags, 10);
#endif

    this->SetSizerAndFit(topSizer.release());

}
