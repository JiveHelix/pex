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
#include "pex/wx/radio_box.h"
#include "pex/wx/view.h"
#include "pex/chooser.h"
#include "units.h"


using Chooser = pex::model::Chooser<UnitsModel, pex::GetTag>;
using ChooserControl = pex::control::Chooser<void, Chooser>;


class ExampleApp: public wxApp
{
public:
    ExampleApp()
        :
        units_{UnitSystem::MKS},
        chooser_{
            this->units_,
            {
                UnitSystem::MKS,
                UnitSystem::CGS,
                UnitSystem::FPS,
                UnitSystem::FFF}}
    {

    }

    bool OnInit() override;

private:
    UnitsModel units_;
    Chooser chooser_;
};


class ExampleFrame: public wxFrame
{
public:
    ExampleFrame(ChooserControl chooser);
};


// Creates the main function for us, and initializes the app's run loop.
wxshimIMPLEMENT_APP(ExampleApp)


bool ExampleApp::OnInit()
{
    ExampleFrame *exampleFrame =
        new ExampleFrame(ChooserControl(this->chooser_));

    exampleFrame->Show();
    return true;
}


ExampleFrame::ExampleFrame(ChooserControl chooserControl)
    :
    wxFrame(nullptr, wxID_ANY, "pex::wx::RadioBox Demo")
{
    auto radioBox =
        new pex::wx::RadioBox<ChooserControl, ShortConverter>(
            this,
            chooserControl,
            "Choose Units");

    auto view =
        new pex::wx::View<ChooserControl::Value, LongConverter>(
            this,
            chooserControl.value);

    auto topSizer = std::make_unique<wxBoxSizer>(wxVERTICAL);

    topSizer->Add(radioBox, 0, wxALL, 10);
    topSizer->Add(view, 0, wxLEFT | wxBOTTOM | wxRIGHT, 10);

    this->SetSizerAndFit(topSizer.release());
}
