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

using IsChecked = pex::model::Value<bool>;
using IsCheckedControl = pex::control::Value<void, IsChecked>;
using Message = pex::model::Value<std::string>;
using MessageControl = pex::control::Value<void, Message>;

class ExampleApp: public wxApp
{
public:
    ExampleApp()
        :
        isChecked_{false},
        message_{"Not checked"},
        isCheckedControl_{this->isChecked_}
    {
        this->isCheckedControl_.Connect(this, &ExampleApp::OnIsChecked_);
    }

    bool OnInit() override;

    void OnIsChecked_(bool value)
    {
        if (value)
        {
            this->message_.Set("Is checked");
        }
        else
        {
            this->message_.Set("Not checked");
        }
    }

private:
    IsChecked isChecked_;
    Message message_;

    pex::control::Value<ExampleApp, IsChecked> isCheckedControl_;
};


class ExampleFrame: public wxFrame
{
public:
    ExampleFrame(IsCheckedControl isChecked, MessageControl message);
};


// Creates the main function for us, and initializes the app's run loop.
wxshimIMPLEMENT_APP(ExampleApp)


bool ExampleApp::OnInit()
{
    ExampleFrame *exampleFrame =
        new ExampleFrame(
            IsCheckedControl(this->isChecked_),
            MessageControl(this->message_));

    exampleFrame->Show();
    return true;
}


ExampleFrame::ExampleFrame(
    IsCheckedControl isChecked,
    MessageControl message)
    :
    wxFrame(nullptr, wxID_ANY, "pex::wx::CheckBox Demo")
{
    auto checkBox =
        new pex::wx::CheckBox<IsCheckedControl>(this, "Check me", isChecked);

    auto view = new pex::wx::View<MessageControl>(this, message);

    auto topSizer = std::make_unique<wxBoxSizer>(wxVERTICAL);

    topSizer->Add(checkBox, 0, wxALL, 10);
    topSizer->Add(view, 0, wxLEFT | wxBOTTOM | wxRIGHT, 10);

    this->SetSizerAndFit(topSizer.release());
}
