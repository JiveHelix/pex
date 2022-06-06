#include "pex/wx/wxshim.h"


#include <tau/color.h>
#include "pex/value.h"
#include "pex/wx/color.h"



class ExampleApp: public wxApp
{
public:
    bool OnInit() override;

private:
    static void OnColor_(void *, const tau::Hsv<float> &color)
    {
        std::cout << "Color:\n" << color << std::endl;
    }

    pex::wx::HsvModel color_ = pex::wx::HsvModel{{{0, 1, 1}}};
};


class ExampleFrame: public wxFrame
{
public:
    ExampleFrame(pex::wx::HsvControl control);
};


// Creates the main function for us, and initializes the app's run loop.
wxshimIMPLEMENT_APP(ExampleApp)


bool ExampleApp::OnInit()
{
    this->color_.Connect(this, &ExampleApp::OnColor_);

    ExampleFrame *exampleFrame =
        new ExampleFrame(pex::wx::HsvControl(this->color_));

    exampleFrame->Show();

    return true;
}


ExampleFrame::ExampleFrame(pex::wx::HsvControl control)
    :
    wxFrame(nullptr, wxID_ANY, "Color Demo")
{
    auto colorPicker = new pex::wx::HsvPicker(this, control);
    auto sizer = std::make_unique<wxBoxSizer>(wxHORIZONTAL);
    
    sizer->Add(colorPicker, 1, wxEXPAND | wxALL, 10);

    this->SetSizerAndFit(sizer.release());
}
