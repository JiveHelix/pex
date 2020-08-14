/**
  * @file combo_box_demo.cpp
  * 
  * @brief A demonstration of pex::wx::ComboBox, backed by
  * a pex::interface::Chooser.
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
using ChooserInterface = pex::interface::Chooser<void, std::string>;

using Firkins = pex::model::Value<bool>;

using FirkinsInterface = pex::interface::Value<void, Firkins>;

using Units = pex::model::Value<std::string>;
using UnitsInterface = pex::interface::Value<void, Units>;


class ExampleApp: public wxApp
{
public:
    ExampleApp()
        :
        unitsChooser_(unitsList),
        firkins_(false),
        units_(this->unitsChooser_.GetSelection())
    {
        this->firkins_.Connect(this, &ExampleApp::OnFirkins_);
        this->unitsChooser_.Connect(this, &ExampleApp::OnSelection_);
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

    static void OnSelection_(void *context, size_t)
    {
        auto self = static_cast<ExampleApp *>(context);
        self->units_.Set(self->unitsChooser_.GetSelection());
    }

    Chooser unitsChooser_;
    Firkins firkins_;
    Units units_;
};


class ExampleFrame: public wxFrame
{
public:
    ExampleFrame(
        ChooserInterface chooserInterface,
        FirkinsInterface firkinsInterface,
        UnitsInterface unitsInterface);
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
            ChooserInterface(&this->unitsChooser_),
            FirkinsInterface(&this->firkins_),
            UnitsInterface(&this->units_));

    exampleFrame->Show();
    return true;
}



ExampleFrame::ExampleFrame(
    ChooserInterface chooserInterface,
    FirkinsInterface firkinsInterface,
    UnitsInterface unitsInterface)
    :
    wxFrame(nullptr, wxID_ANY, "pex::wx::ComboBox Demo")
{
    auto firkinsCheckbox =
        new pex::wx::CheckBox<FirkinsInterface>(
            this,
            "Show FFF",
            firkinsInterface);

    auto comboBox =
        new pex::wx::ComboBox<std::string>(this, chooserInterface);

    auto view = new pex::wx::View<UnitsInterface>(this, unitsInterface);

    wxBoxSizer *topSizer = new wxBoxSizer(wxVERTICAL); 

    topSizer->Add(firkinsCheckbox, 0, wxALL, 10);
    topSizer->Add(comboBox, 0, wxLEFT | wxBOTTOM | wxRIGHT, 10);
    topSizer->Add(view, 0, wxLEFT | wxBOTTOM | wxRIGHT, 10);
    this->SetSizerAndFit(topSizer);
}
