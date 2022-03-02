/**
  * @file slider_demo.cpp
  * 
  * @brief A demonstration of pex::wx::Slider.
  * 
  * @author Jive Helix (jivehelix@gmail.com)
  * @date 17 Aug 2020
  * @copyright Jive Helix
  * Licensed under the MIT license. See LICENSE file.
**/

#include "pex/range.h"
#include "pex/wx/slider.h"


using Position = pex::model::Range<int>;

inline constexpr size_t defaultPosition = 0;
inline constexpr size_t minimumPosition = 0;
inline constexpr size_t maximumPosition = 1000;


using PlaybackSpeed = pex::model::Range<float>;

inline constexpr float minimumPlaybackSpeed = 0.125f;
inline constexpr float maximumPlaybackSpeed = 2.0f;
inline constexpr float defaultPlaybackSpeed = 1.0f;


class ExampleApp : public wxApp
{
public:
    ExampleApp()
        :
        position_(defaultPosition, minimumPosition, maximumPosition),
        playbackSpeed_(
            defaultPlaybackSpeed,
            minimumPlaybackSpeed,
            maximumPlaybackSpeed)
    {

    }

    bool OnInit() override;

private:
    Position position_;
    PlaybackSpeed playbackSpeed_;
};


// Do not filter the position.
using PositionControl = pex::control::Range<void, Position>;

// This is the control node used to display position's value.
using PositionValue = pex::control::Value<void, Position::Value>;


/**
 ** Create a filter that converts between a logarithmic value and a linear one.
 **/
template<typename T>
struct PlaybackSpeedFilter
{
    static constexpr auto base = static_cast<T>(2.0);

    // Higher divisor increases the integer range of the filter, which gives
    // the slider finer control.
    static constexpr auto divisor = static_cast<T>(100.0);

    static int Get(T value)
    {
        return static_cast<int>(round(divisor * std::log2(value)));
    }

    static T Set(int value)
    {
        return static_cast<T>(powf(base, static_cast<T>(value) / divisor));
    }
};


using PlaybackSpeedControl = ::pex::control::Range<
    void,
    PlaybackSpeed,
    PlaybackSpeedFilter<float>>;

// The unfiltered value used to display playback speed
using PlaybackSpeedValue =
    pex::control::Value<void, typename PlaybackSpeed::Value>;


const int precision = 3;

using PositionSlider =
    pex::wx::SliderAndValue<PositionControl, PositionValue, precision>;


using PlaybackSpeedSlider =
    pex::wx::SliderAndValue<PlaybackSpeedControl, PlaybackSpeedValue>;


class ExampleFrame: public wxFrame
{
public:
    ExampleFrame(
        PositionControl positionRange,
        PositionValue positionValue,
        PlaybackSpeedControl playbackSpeedRange,
        PlaybackSpeedValue playbackSpeedValue);
};


// Creates the main function for us, and initializes the app's run loop.
wxshimIMPLEMENT_APP(ExampleApp)


bool ExampleApp::OnInit()
{
    ExampleFrame *exampleFrame =
        new ExampleFrame(
            PositionControl(this->position_),
            PositionValue(this->position_.GetValueControl()),
            PlaybackSpeedControl(this->playbackSpeed_),
            PlaybackSpeedValue(this->playbackSpeed_.GetValueControl()));

    exampleFrame->Show();
    return true;
}



ExampleFrame::ExampleFrame(
    PositionControl positionRange,
    PositionValue positionValue,
    PlaybackSpeedControl playbackSpeedRange,
    PlaybackSpeedValue playbackSpeedValue)
    :
    wxFrame(nullptr, wxID_ANY, "pex::wx::Slider Demo")
{
    auto positionSlider =
        new PositionSlider(this, positionRange, positionValue);

    auto playbackSpeedSlider =
        new PlaybackSpeedSlider(this, playbackSpeedRange, playbackSpeedValue);

    auto speedView =
        new pex::wx::View<PlaybackSpeedValue>(this, playbackSpeedValue);

    auto topSizer = std::make_unique<wxBoxSizer>(wxVERTICAL);

    topSizer->Add(positionSlider, 0, wxALL | wxEXPAND, 10);
    topSizer->Add(playbackSpeedSlider, 0, wxALL | wxEXPAND, 10);
    topSizer->Add(speedView, 0, wxALL | wxEXPAND, 10);

    this->SetSizerAndFit(topSizer.release());
}
