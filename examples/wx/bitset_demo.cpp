/**
  * @file bitset_demo_with_filter.cpp
  * 
  * @brief A simpler demonstration of a bitset Field, using FlagFilter to
  * convert between bitset and boolean flags.
  * 
  * @author Jive Helix (jivehelix@gmail.com)
  * @date 17 Aug 2020
  * @copyright Jive Helix
  * Licensed under the MIT license. See LICENSE file.
**/

#include <iostream>
#include <string>
#include "pex/wx/wxshim.h"
#include "pex/wx/view.h"
#include "pex/wx/field.h"
#include "pex/wx/bitset_check_boxes.h"
#include "pex/wx/labeled_widget.h"


constexpr auto bitCount = 5;
using Bitset = std::bitset<bitCount>;

using BitsetModel = pex::model::Value<Bitset>;
using BitsetControl = pex::control::Value<void, BitsetModel>;


struct Model
{
    BitsetModel bitset;

    Model()
        :
        bitset{}
    {

    }
};


struct Control
{
    BitsetControl bitset;
    pex::wx::BitsetFlagsControl<bitCount> flags;

    Control(Model &model)
        :
        bitset(model.bitset),
        flags(model.bitset)
    {

    }
};


class ExampleApp: public wxApp
{
public:
    ExampleApp()
        :
        model_{},
        control_(this->model_)
    {

    }

    bool OnInit() override;

private:
    Model model_;
    Control control_;
};


class ExampleFrame: public wxFrame
{
public:
    ExampleFrame(Control control);
};


// Creates the main function for us, and initializes the app's run loop.
wxshimIMPLEMENT_APP(ExampleApp)


bool ExampleApp::OnInit()
{
    ExampleFrame *exampleFrame = new ExampleFrame(this->control_);

    exampleFrame->Show();
    return true;
}


ExampleFrame::ExampleFrame(Control control)
    :
    wxFrame(nullptr, wxID_ANY, "Bitset Demo")
{
    using namespace pex::wx;

    auto bitsetView =
        LabeledWidget(
            this,
            MakeWidgetMaker<View>(control.bitset),
            "Bitset (view):");

    auto bitsetField =
        LabeledWidget(
            this,
            MakeWidgetMaker<Field>(control.bitset),
            "Bitset (field):");

    auto pointSize = bitsetView.GetLabel()->GetFont().GetPointSize();
    wxFont font(wxFontInfo(pointSize).Family(wxFONTFAMILY_TELETYPE));
    bitsetView.GetWidget()->SetFont(font);
    bitsetField.GetWidget()->SetFont(font);

    auto bitsetFlags =
        LabeledWidget(
            this,
            MakeBitsetCheckBoxes<bitCount>
            {
                control.flags,
            },
            "Bitset (default names):");

    auto bitsetFlagsCustomized =
        LabeledWidget(
            this,
            MakeBitsetCheckBoxes<bitCount>
            {
                control.flags,
                {"Enable", "Filter", "Fast", "Slow", "?"}
            },
            "Bitset (customized names):");

    auto topSizer = std::make_unique<wxBoxSizer>(wxVERTICAL);

    LayoutOptions options{};

    options.orient = wxVERTICAL;
    options.labelAlign = wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL;

    auto layoutSizer = LayoutLabeled(
        options,
        bitsetView,
        bitsetField,
        bitsetFlags,
        bitsetFlagsCustomized);

    topSizer->Add(layoutSizer.release(), 0, wxALL, 10);
    this->SetSizerAndFit(topSizer.release());
}
