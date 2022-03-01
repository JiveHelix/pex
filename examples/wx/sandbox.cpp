/**
  * @file sandbox.cpp
  *
  * @brief A sandbox for experimentation.
  *
  * @author Jive Helix (jivehelix@gmail.com)
  * @date 07 Aug 2020
  * @copyright Jive Helix
  * Licensed under the MIT license. See LICENSE file.
**/

#include <iostream>
#include <string>
#include "pex/wx/wxshim.h"
#include "jive/formatter.h"
#include "tau/angles.h"
#include "pex/signal.h"
#include "pex/value.h"
#include "pex/converter.h"
#include "pex/wx/view.h"
#include "pex/wx/knob.h"


using Angle = pex::model::Value<double>;
using Interface = pex::interface::Value<void, Angle>;


class ExampleApp: public wxApp
{
public:
    ExampleApp()
        :
        angle_{0.0},
    {

    }

    bool OnInit() override;

private:
    Angle angle_;
};


class ExampleFrame: public wxFrame
{
public:
    ExampleFrame(Interface value);
};


// Creates the main function for us, and initializes the app's run loop.
wxshimIMPLEMENT_APP(ExampleApp)


bool ExampleApp::OnInit()
{
    ExampleFrame *exampleFrame = new ExampleFrame(Interface(&this->angle_));
    exampleFrame->Show();
    return true;
}


ExampleFrame::ExampleFrame(Interface interface)
    :
    wxFrame(nullptr, wxID_ANY, "pex::wx::Sandbox")
{
    // Any instance constructed with a parent automatically becomes owned by
    // the parent, so there is no need to protect it with unique_ptr.
    auto view = new pex::wx::View<Interface>(this, interface);
    auto knob = new pex::wx::Knob<Interface>(this, interface);

    // Sizers are not owned until a call to SetSizer.
    // unique_ptr will manage it until then.
    auto topSizer = std::make_unique<wxBoxSizer>(wxVERTICAL);
    auto flags = wxLEFT | wxBOTTOM | wxRIGHT | wxEXPAND;

    topSizer->Add(view, 0, wxALL, 10);

    this->SetSizerAndFit(topSizer.release());
}
