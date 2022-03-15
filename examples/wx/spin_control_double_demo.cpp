/**
  * @file spin_control_double_demo.cpp
  * 
  * @brief A demonstration of pex::wx::SpinControlDouble.
  * 
  * @author Jive Helix (jivehelix@gmail.com)
  * @date 17 Aug 2020
  * @copyright Jive Helix
  * Licensed under the MIT license. See LICENSE file.
**/

#include "pex/range.h"
#include "pex/wx/view.h"
#include "pex/wx/spin_control_double.h"

using Wibble = pex::model::Value<double>;
using WibbleRange = pex::model::Range<Wibble>;
using WibbleRangeControl = pex::control::Range<void, WibbleRange>;

using WibbleValue = typename WibbleRangeControl::Value;


inline constexpr double defaultWibble = 10;
inline constexpr double minimumWibble = 0;
inline constexpr double maximumWibble = 20;
inline constexpr double wibbleIncrement = 1;


using Wobble = pex::model::Value<float>;
using WobbleRange = pex::model::Range<Wobble>;

using WobbleRangeControl = pex::control::Range<void, WobbleRange>;

using WobbleValue = typename WobbleRangeControl::Value;


inline constexpr float defaultWobble = 0;
inline constexpr float minimumWobble = -100.0;
inline constexpr float maximumWobble = 100;
inline constexpr float wobbleIncrement = 2.5;


class ExampleApp : public wxApp
{
public:
    ExampleApp()
        :
        wibble_(defaultWibble),
        wibbleRange_(this->wibble_),
        wobble_(defaultWobble),
        wobbleRange_(this->wobble_)
    {
        this->wibbleRange_.SetLimits(minimumWibble, maximumWibble);
        this->wobbleRange_.SetLimits(minimumWobble, maximumWobble);
    }

    bool OnInit() override;

private:
    Wibble wibble_;
    WibbleRange wibbleRange_;
    Wobble wobble_;
    WobbleRange wobbleRange_;
};




class ExampleFrame: public wxFrame
{
public:
    ExampleFrame(
        WibbleRangeControl wibbleRange,
        WibbleValue wibbleValue,
        WobbleRangeControl wobbleRange,
        WobbleValue wobbleValue);
};


// Creates the main function for us, and initializes the app's run loop.
wxshimIMPLEMENT_APP(ExampleApp)


bool ExampleApp::OnInit()
{
    auto wibble = WibbleRangeControl(this->wibbleRange_);
    auto wobble = WobbleRangeControl(this->wobbleRange_);

    ExampleFrame *exampleFrame =
        new ExampleFrame(
            wibble,
            wibble.value,
            wobble,
            wobble.value);

    exampleFrame->Show();
    return true;
}


using WibbleSpinControl = pex::wx::SpinControlDouble<WibbleRangeControl>;
using WobbleSpinControl = pex::wx::SpinControlDouble<WobbleRangeControl>;


ExampleFrame::ExampleFrame(
        WibbleRangeControl wibbleRange,
        WibbleValue wibbleValue,
        WobbleRangeControl wobbleRange,
        WobbleValue wobbleValue)
    :
    wxFrame(nullptr, wxID_ANY, "pex::wx::SpinControlDouble Demo")
{
    auto wibbleView =
        new pex::wx::View(this, wibbleValue);

    auto wibbleSpinControl =
        new WibbleSpinControl(this, wibbleRange, wibbleIncrement, 0);

    auto wobbleView =
        new pex::wx::View(this, wobbleValue);

    auto wobbleSpinControl =
        new WobbleSpinControl(this, wobbleRange, wobbleIncrement);

    auto topSizer = std::make_unique<wxBoxSizer>(wxVERTICAL);

    topSizer->Add(wibbleSpinControl, 0, wxALL | wxEXPAND, 10);
    topSizer->Add(wibbleView, 0, wxALL | wxEXPAND, 10);
    topSizer->Add(wobbleSpinControl, 0, wxALL | wxEXPAND, 10);
    topSizer->Add(wobbleView, 0, wxALL | wxEXPAND, 10);
    this->SetSizerAndFit(topSizer.release());
}
