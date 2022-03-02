/**
  * @file radio_box_demo.cpp
  *
  * @brief Demonstrates the usage of pex::wx::RadioBox.
  *
  * @author Jive Helix (jivehelix@gmail.com)
  * @date 07 Aug 2020
  * @copyright Jive Helix
  * Licensed under the MIT license. See LICENSE file.
**/

#include "pex/wx/view.h"
#include "pex/value.h"
#include "pex/wx/radio_box.h"
#include "pex/wx/view.h"


enum class UnitSystem: uint8_t
{
    MKS,
    CGS,
    FPS,
    FFF
};


struct ShortConverter
{
    static std::string ToString(UnitSystem unitSystem)
    {
        switch (unitSystem)
        {
            case (UnitSystem::MKS):
                return "MKS";

            case (UnitSystem::CGS):
                return "CGS";

            case (UnitSystem::FPS):
                return "FPS";

            case (UnitSystem::FFF):
                return "FFF";

            default:
                throw std::logic_error("Unknown unit system");
        }
    }
};


struct LongConverter
{
    static std::string ToString(UnitSystem unitSystem)
    {
        switch (unitSystem)
        {
            case (UnitSystem::MKS):
                return "meter-kilogram-second";

            case (UnitSystem::CGS):
                return "centimeter-gram-second";

            case (UnitSystem::FPS):
                return "foot-pound-second";

            case (UnitSystem::FFF):
                return "furlong-firkin-fortnight";

            default:
                throw std::logic_error("Unknown unit system");
        }
    }
};


using UnitsModel = pex::model::Value<UnitSystem>;
using UnitsControl = pex::control::Value<void, UnitsModel>;

class ExampleApp: public wxApp
{
public:
    ExampleApp()
        :
        units_{UnitSystem::MKS}
    {

    }

    bool OnInit() override;

private:
    UnitsModel units_;
};


class ExampleFrame: public wxFrame
{
public:
    ExampleFrame(UnitsControl units);
};


// Creates the main function for us, and initializes the app's run loop.
wxshimIMPLEMENT_APP(ExampleApp)


bool ExampleApp::OnInit()
{
    ExampleFrame *exampleFrame =
        new ExampleFrame(UnitsControl(this->units_));

    exampleFrame->Show();
    return true;
}


auto choices = std::vector<UnitSystem>
{
    UnitSystem::MKS,
    UnitSystem::CGS,
    UnitSystem::FPS,
    UnitSystem::FFF
};


ExampleFrame::ExampleFrame(UnitsControl unitsControl)
    :
    wxFrame(nullptr, wxID_ANY, "pex::wx::RadioBox Demo")
{
    auto radioBox =
        new pex::wx::RadioBox<UnitsControl, ShortConverter>(
            this,
            unitsControl,
            choices,
            "Choose Units");

    auto view =
        new pex::wx::View<UnitsControl, LongConverter>(
            this,
            unitsControl);

    auto topSizer = std::make_unique<wxBoxSizer>(wxVERTICAL);

    topSizer->Add(radioBox, 0, wxALL, 10);
    topSizer->Add(view, 0, wxLEFT | wxBOTTOM | wxRIGHT, 10);

    this->SetSizerAndFit(topSizer.release());
}
