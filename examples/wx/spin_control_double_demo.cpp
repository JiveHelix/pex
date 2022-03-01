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

using Wibble = pex::model::Range<double>;

inline constexpr double defaultWibble = 10;
inline constexpr double minimumWibble = 0;
inline constexpr double maximumWibble = 20;
inline constexpr double wibbleIncrement = 1;


using Wobble = pex::model::Range<float>;

inline constexpr float defaultWobble = 0;
inline constexpr float minimumWobble = -100.0;
inline constexpr float maximumWobble = 100;
inline constexpr float wobbleIncrement = 2.5;


class ExampleApp : public wxApp
{
public:
    ExampleApp()
        :
        wibble_(defaultWibble, minimumWibble, maximumWibble),
        wobble_(defaultWobble, minimumWobble, maximumWobble)
    {

    }

    bool OnInit() override;

private:
    Wibble wibble_;
    Wobble wobble_;
};


using WibbleSpinControl = pex::wx::SpinControlDouble<Wibble>;
using WobbleSpinControl = pex::wx::SpinControlDouble<Wobble>;

using WibbleRange = pex::interface::Range<void, Wibble>;
using WobbleRange = pex::interface::Range<void, Wobble>;

using WibbleValue = pex::interface::Value<void, typename Wibble::Value>;
using WobbleValue = pex::interface::Value<void, typename Wobble::Value>;


class ExampleFrame: public wxFrame
{
public:
    ExampleFrame(
        WibbleRange wibbleRange,
        WibbleValue wibbleValue,
        WobbleRange wobbleRange,
        WobbleValue wobbleValue);
};


// Creates the main function for us, and initializes the app's run loop.
wxshimIMPLEMENT_APP(ExampleApp)


bool ExampleApp::OnInit()
{
    ExampleFrame *exampleFrame =
        new ExampleFrame(
            WibbleRange(&this->wibble_),
            WibbleValue(this->wibble_.GetValueInterface()),
            WobbleRange(&this->wobble_),
            WobbleValue(this->wobble_.GetValueInterface()));

    exampleFrame->Show();
    return true;
}



ExampleFrame::ExampleFrame(
        WibbleRange wibbleRange,
        WibbleValue wibbleValue,
        WobbleRange wobbleRange,
        WobbleValue wobbleValue)
    :
    wxFrame(nullptr, wxID_ANY, "pex::wx::SpinControlDouble Demo")
{
    auto wibbleView =
        new pex::wx::View<WibbleValue>(this, wibbleValue);

    auto wibbleSpinControl =
        new WibbleSpinControl(this, wibbleRange, wibbleIncrement, 0);

    auto wobbleView =
        new pex::wx::View<WobbleValue>(this, wobbleValue);

    auto wobbleSpinControl =
        new WobbleSpinControl(this, wobbleRange, wobbleIncrement);

    auto topSizer = std::make_unique<wxBoxSizer>(wxVERTICAL);

    topSizer->Add(wibbleSpinControl, 0, wxALL | wxEXPAND, 10);
    topSizer->Add(wibbleView, 0, wxALL | wxEXPAND, 10);
    topSizer->Add(wobbleSpinControl, 0, wxALL | wxEXPAND, 10);
    topSizer->Add(wobbleView, 0, wxALL | wxEXPAND, 10);
    this->SetSizerAndFit(topSizer.release());
}
