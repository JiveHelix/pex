#include "pex/wx/wxshim.h"


#include <tau/color.h>
#include "pex/value.h"
#include "pex/wx/color.h"



class ExampleApp: public wxApp
{
public:
    bool OnInit() override;

private:
    void OnColor_(const tau::Hsv<float> &color)
    {
        std::cout << "Color:\n" << color << std::endl;
    }

    pex::wx::HsvModel color_ = pex::wx::HsvModel{{{0, 1, 1}}};

    using ColorControl = pex::Terminus<ExampleApp, pex::wx::HsvModel>;
    ColorControl colorControl_;
};


class ExampleFrame: public wxFrame
{
public:
    ExampleFrame(pex::wx::HsvControl control);
};


// Creates the main function for us, and initializes the app's run loop.
wxshimIMPLEMENT_APP_CONSOLE(ExampleApp)


bool ExampleApp::OnInit()
{
    this->colorControl_.Assign(ColorControl(this, this->color_), this);

    PEX_LOG("color_.Connect");
    this->colorControl_.Connect(&ExampleApp::OnColor_);

    PEX_LOG("ExampleFrame");
    ExampleFrame *exampleFrame =
        new ExampleFrame(pex::wx::HsvControl(this->color_));

    exampleFrame->Show();

    return true;
}


ExampleFrame::ExampleFrame(pex::wx::HsvControl control)
    :
    wxFrame(nullptr, wxID_ANY, "Color Demo")
{
    PEX_LOG("\n\n ********* new HsvPicker ************* \n\n");
    auto colorPicker = new pex::wx::HsvPicker(this, control);
    auto sizer = std::make_unique<wxBoxSizer>(wxHORIZONTAL);
    
    sizer->Add(colorPicker, 1, wxEXPAND | wxALL, 10);

    this->SetSizerAndFit(sizer.release());
}
