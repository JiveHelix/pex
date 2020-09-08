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


using Position = pex::model::Range<size_t>;

inline constexpr size_t defaultPosition = 0;
inline constexpr size_t minimumPosition = 0;
inline constexpr size_t maximumPosition = 1000;


template<typename T>
struct PlaybackSpeedFilter
{
    static constexpr auto base = static_cast<T>(2.0);
    static constexpr auto divisor = static_cast<T>(20.0);

    static int Get(T value)
    {
        return static_cast<int>(round(divisor * std::log2(value)));
    }

    static T Set(int value)
    {
        return static_cast<T>(powf(base, static_cast<T>(value) / divisor));
    }
};


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

using PositionValue =
    pex::interface::Value<void, typename Position::Value>;

using PlaybackSpeedValue =
    pex::interface::Value<void, typename PlaybackSpeed::Value>;

// Use the default filter for position.
using PositionSlider =
    pex::wx::SliderAndValue<Position, PositionValue>;

using PlaybackSpeedSlider =
    pex::wx::Slider
    <
        PlaybackSpeed,
        PlaybackSpeedFilter<float>
    >;

using PositionRange = ::pex::interface::Range<void, Position>;
using PlaybackSpeedRange = ::pex::interface::Range<void, PlaybackSpeed>;

class ExampleFrame: public wxFrame
{
public:
    ExampleFrame(
        PositionRange positionRange,
        PositionValue positionValue,
        PlaybackSpeedRange playbackSpeedRange,
        PlaybackSpeedValue playbackSpeedValue);
};

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"

// Creates the main function for us, and initializes the app's run loop.
wxIMPLEMENT_APP(ExampleApp);

#pragma GCC diagnostic pop


bool ExampleApp::OnInit()
{
    ExampleFrame *exampleFrame =
        new ExampleFrame(
            PositionRange(&this->position_),
            PositionValue(this->position_.GetValueInterface()),
            PlaybackSpeedRange(&this->playbackSpeed_),
            PlaybackSpeedValue(this->playbackSpeed_.GetValueInterface()));

    exampleFrame->Show();
    return true;
}



ExampleFrame::ExampleFrame(
    PositionRange positionRange,
    PositionValue positionValue,
    PlaybackSpeedRange playbackSpeedRange,
    PlaybackSpeedValue playbackSpeedValue)
    :
    wxFrame(nullptr, wxID_ANY, "pex::wx::Slider Demo")
{
    auto positionSlider =
        new PositionSlider(this, positionRange, positionValue);

    auto playbackSpeedSlider =
        new PlaybackSpeedSlider(this, playbackSpeedRange);

    auto speedView =
        new pex::wx::View<PlaybackSpeedValue>(this, playbackSpeedValue);

    auto topSizer = std::make_unique<wxBoxSizer>(wxVERTICAL);

    topSizer->Add(positionSlider, 0, wxALL | wxEXPAND, 10);
    topSizer->Add(playbackSpeedSlider, 0, wxALL | wxEXPAND, 10);
    topSizer->Add(speedView, 0, wxALL | wxEXPAND, 10);

    this->SetSizerAndFit(topSizer.release());
}
