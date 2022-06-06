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
#include "units.h"


static inline const std::vector<UnitSystem> withoutFirkins
{
    UnitSystem::MKS,
    UnitSystem::CGS,
    UnitSystem::FPS
};


static inline const std::vector<UnitSystem> withFirkins
{
    UnitSystem::MKS,
    UnitSystem::CGS,
    UnitSystem::FPS,
    UnitSystem::FFF
};


std::string fffUnits = "furlong-firkin-fortnight";

using Chooser = pex::model::Chooser<UnitsModel>;
using ChooserControl = pex::control::Chooser<void, Chooser>;

using EnableFirkins = pex::model::Value<bool>;
using EnableFirkinsControl = pex::control::Value<void, EnableFirkins>;


class ExampleApp: public wxApp
{
public:
    ExampleApp()
        :
        units_{UnitSystem::MKS},
        chooser_{
            this->units_,
            withoutFirkins},
        enableFirkins_(false)
    {
        this->enableFirkins_.Connect(this, &ExampleApp::OnFirkins_);
    }
    
    ~ExampleApp()
    {
        this->enableFirkins_.Disconnect(this);
    }

    bool OnInit() override;

private:
    static void OnFirkins_(void * context, bool firkins)
    {
        auto self = static_cast<ExampleApp *>(context);

        if (firkins)
        {
            self->chooser_.SetChoices(withFirkins);
        }
        else
        {
            self->chooser_.SetChoices(withoutFirkins);
        }
    }

    UnitsModel units_;
    Chooser chooser_;
    EnableFirkins enableFirkins_;
};


class ExampleFrame: public wxFrame
{
public:
    ExampleFrame(
        ChooserControl chooserControl,
        EnableFirkinsControl enableFirkinsControl);
};


// Creates the main function for us, and initializes the app's run loop.
wxshimIMPLEMENT_APP(ExampleApp)


bool ExampleApp::OnInit()
{
    ExampleFrame *exampleFrame =
        new ExampleFrame(
            ChooserControl(this->chooser_),
            EnableFirkinsControl(this->enableFirkins_));

    exampleFrame->Show();
    return true;
}


ExampleFrame::ExampleFrame(
    ChooserControl chooserControl,
    EnableFirkinsControl enableFirkinsControl)
    :
    wxFrame(nullptr, wxID_ANY, "pex::wx::ComboBox Demo")
{
    auto firkinsCheckbox =
        new pex::wx::CheckBox(
            this,
            "Show FFF",
            enableFirkinsControl);

    auto shortComboBox =
        new pex::wx::ComboBox<ChooserControl, ShortConverter>(
            this,
            chooserControl);

    auto longComboBox =
        new pex::wx::ComboBox<ChooserControl, LongConverter>(
            this,
            chooserControl);

    auto shortView =
        new pex::wx::View<ChooserControl::Value, ShortConverter>(
            this,
            chooserControl.value);

    auto longView =
        new pex::wx::View<ChooserControl::Value, LongConverter>(
            this,
            chooserControl.value);

    auto topSizer = std::make_unique<wxBoxSizer>(wxVERTICAL);

    topSizer->Add(firkinsCheckbox, 0, wxALL, 10);
    topSizer->Add(shortComboBox, 0, wxLEFT | wxBOTTOM | wxRIGHT, 10);
    topSizer->Add(longComboBox, 0, wxLEFT | wxBOTTOM | wxRIGHT, 10);
    topSizer->Add(shortView, 0, wxLEFT | wxBOTTOM | wxRIGHT, 10);
    topSizer->Add(longView, 0, wxLEFT | wxBOTTOM | wxRIGHT, 10);

    this->SetSizerAndFit(topSizer.release());
}
