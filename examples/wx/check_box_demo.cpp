/**
  * @file check_box_demo.cpp
  *
  * @brief Demonstrates the usage of pex::wx::CheckBox.
  *
  * @author Jive Helix (jivehelix@gmail.com)
  * @date 07 Aug 2020
  * @copyright Jive Helix
  * Licensed under the MIT license. See LICENSE file.
**/

#include "pex/wx/view.h"
#include "pex/value.h"
#include "pex/wx/check_box.h"
#include "pex/wx/view.h"
#include "pex/group.h"


template<typename T>
struct DemoFields
{
    static constexpr auto fields = std::make_tuple(
        fields::Field(&T::isChecked, "isChecked"),
        fields::Field(&T::message, "message"));
};


template<template<typename> typename T>
struct DemoTemplate
{
    T<bool> isChecked;
    T<std::string> message;
};


using DemoGroup = pex::Group<DemoFields, DemoTemplate>;

template<typename Observer>
using DemoControl = typename DemoGroup::Control<Observer>;

using DemoModel = typename DemoGroup::Model;


using IsCheckedControl = decltype(DemoControl<void>::isChecked);


class ExampleApp: public wxApp
{
public:
    ExampleApp()
        :
        model_{{{false, "Not checked"}}},
        isChecked{this, this->model_.isChecked}
    {
        this->isChecked.Connect(&ExampleApp::OnIsChecked_);
    }

    bool OnInit() override;

private:
    void OnIsChecked_(bool value)
    {
        if (value)
        {
            this->model_.message.Set("Is checked");
        }
        else
        {
            this->model_.message.Set("Not checked");
        }
    }

private:
    DemoModel model_;
    pex::Terminus<ExampleApp, IsCheckedControl> isChecked;
};


class ExampleFrame: public wxFrame
{
public:
    ExampleFrame(DemoControl<void> control);
};


// Creates the main function for us, and initializes the app's run loop.
wxshimIMPLEMENT_APP(ExampleApp)


bool ExampleApp::OnInit()
{
    ExampleFrame *exampleFrame =
        new ExampleFrame(DemoControl<void>(this->model_));

    exampleFrame->Show();
    return true;
}


ExampleFrame::ExampleFrame(DemoControl<void> control)
    :
    wxFrame(nullptr, wxID_ANY, "pex::wx::CheckBox Demo")
{
    auto checkBox =
        new pex::wx::CheckBox(this, "Check me", control.isChecked);

    auto view = new pex::wx::View(this, control.message);

    auto topSizer = std::make_unique<wxBoxSizer>(wxVERTICAL);

    topSizer->Add(checkBox, 0, wxALL, 10);
    topSizer->Add(view, 0, wxLEFT | wxBOTTOM | wxRIGHT, 10);

    this->SetSizerAndFit(topSizer.release());
}
