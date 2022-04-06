/**
  * @file combo_box_demo.cpp
  *
  * @brief A demonstration of pex::wx::ComboBox, backed by
  * a pex::control::Chooser.
  *
  * @author Jive Helix (jivehelix@gmail.com)
  * @date 14 Aug 2020
  * @copyright Jive Helix
  * Licensed under the MIT license. See LICENSE file.
**/


#include <string>
#include <vector>
#include "pex/chooser.h"
#include "pex/wx/combo_box.h"
#include "pex/wx/check_box.h"
#include "pex/wx/view.h"


static inline const std::vector<std::string> unitsList
{
    "meter-kilogram-second",
    "centimeter-gram-second",
    "foot-pound-second"
};


std::string fffUnits = "furlong-firkin-fortnight";

using Chooser = pex::model::Chooser<std::string>;
using ChooserControl = pex::control::Chooser<void, Chooser>;

using Firkins = pex::model::Value<bool>;

using FirkinsControl = pex::control::Value<void, Firkins>;

using Units = pex::model::Value<std::string>;
using UnitsControl = pex::control::Value<void, Units>;


class ExampleApp: public wxApp
{
public:
    ExampleApp()
        :
        unitsChooser_(unitsList),
        firkins_(false),
        units_(this->unitsChooser_.GetSelection()),
        link_(pex::MakeLink(this->unitsChooser_, this->units_))
    {
        this->firkins_.Connect(this, &ExampleApp::OnFirkins_);
    }
    
    ~ExampleApp()
    {
        this->firkins_.Disconnect(this);
    }

    bool OnInit() override;

private:
    static void OnFirkins_(void * context, bool firkins)
    {
        auto self = static_cast<ExampleApp *>(context);

        if (firkins)
        {
            auto firkinsList = unitsList;
            firkinsList.push_back(fffUnits);
            self->unitsChooser_.SetChoices(firkinsList);
        }
        else
        {
            self->unitsChooser_.SetChoices(unitsList);
        }
    }

    Chooser unitsChooser_;
    Firkins firkins_;
    Units units_;
    std::unique_ptr<pex::Link> link_;
};


class ExampleFrame: public wxFrame
{
public:
    ExampleFrame(
        ChooserControl chooserControl,
        FirkinsControl firkinsControl,
        UnitsControl unitsControl);
};


// Creates the main function for us, and initializes the app's run loop.
wxshimIMPLEMENT_APP(ExampleApp)


bool ExampleApp::OnInit()
{
    ExampleFrame *exampleFrame =
        new ExampleFrame(
            ChooserControl(this->unitsChooser_),
            FirkinsControl(this->firkins_),
            UnitsControl(this->units_));

    exampleFrame->Show();
    return true;
}



ExampleFrame::ExampleFrame(
    ChooserControl chooserControl,
    FirkinsControl firkinsControl,
    UnitsControl unitsControl)
    :
    wxFrame(nullptr, wxID_ANY, "pex::wx::ComboBox Demo")
{
    auto firkinsCheckbox =
        new pex::wx::CheckBox(
            this,
            "Show FFF",
            firkinsControl);

    auto comboBox =
        new pex::wx::ComboBox(this, chooserControl);

    auto view = new pex::wx::View(this, unitsControl);
    auto topSizer = std::make_unique<wxBoxSizer>(wxVERTICAL);

    topSizer->Add(firkinsCheckbox, 0, wxALL, 10);
    topSizer->Add(comboBox, 0, wxLEFT | wxBOTTOM | wxRIGHT, 10);
    topSizer->Add(view, 0, wxLEFT | wxBOTTOM | wxRIGHT, 10);

    this->SetSizerAndFit(topSizer.release());
}
