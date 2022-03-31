/**
  * @file async_demo.cpp
  * 
  * @brief Demonstrates asynchronous communcation from a worker thread.
  * 
  * @author Jive Helix (jivehelix@gmail.com)
  * @date 22 Mar 2022
  * @copyright Jive Helix
  * Licensed under the MIT license. See LICENSE file.
**/


#include <string>
#include <thread>
#include <chrono>
#include <tau/angles.h>
#include <jive/future.h>

#include "pex/value.h"
#include "pex/signal.h"
#include "pex/wx/wxshim.h"
#include "pex/wx/view.h"
#include "pex/wx/field.h"
#include "pex/wx/labeled_widget.h"
#include "pex/wx/async.h"
#include "pex/wx/button.h"


using AngleRadians = pex::model::Value<double>;

template<typename Upstream>
using RadiansControl = pex::control::Value<void, Upstream>;

template<typename Upstream>
auto MakeRadiansControl(Upstream &upstream)
{
    return RadiansControl<Upstream>(upstream);
}

/** Allow a control to use degrees, while the model uses radians. **/
struct DegreesFilter
{
    /** Convert to degrees on retrieval **/
    static double Get(double value)
    {
        return tau::ToDegrees(value);
    }

    /** Convert back to radians on assignment **/
    static double Set(double value)
    {
        return tau::ToRadians(value);
    }
};


template<typename Upstream>
using DegreesControl =
    pex::control::FilteredValue<void, Upstream, DegreesFilter>;


template<typename Upstream>
auto MakeDegreesControl(Upstream &upstream)
{
    return DegreesControl<Upstream>(upstream);
}


class ExampleApp: public wxApp
{
public:
    ExampleApp()
        :
        mutex_{},
        startingAngle_{0.0},
        currentAngle_{},
        start_{},
        stop_{},
        isRunning_{},
        worker_{}
    {
        this->startingAngle_.Connect(this, &ExampleApp::OnUpdate_);
        this->start_.Connect(this, &ExampleApp::OnStart_);
        this->stop_.Connect(this, &ExampleApp::OnStop_);
    }

    bool OnInit() override;

private:
    static void OnUpdate_(void *context, double value)
    {
        auto self = reinterpret_cast<ExampleApp *>(context);
        auto control = self->currentAngle_.GetWxControl();
        control.Set(value);
    }

    static void OnStart_(void *context)
    {
        auto self = reinterpret_cast<ExampleApp *>(context);

        if (self->isRunning_)
        {
            return;
        }

        self->isRunning_ = true;

        self->worker_ = std::thread(
            std::bind(&ExampleApp::WorkerThread_, self));
    }

    static void OnStop_(void *context)
    {
        auto self = reinterpret_cast<ExampleApp *>(context);

        if (!self->isRunning_)
        {
            return;
        }

        self->isRunning_ = false;
        self->worker_.join();
    }

    void WorkerThread_()
    {
        auto workerControl = this->currentAngle_.GetWorkerControl();

        while (this->isRunning_)
        {
            auto next =
                this->currentAngle_.Get() + tau::Angles<double>::pi / 4.0;

            workerControl.Set(next);

            using namespace std::chrono_literals;
            std::this_thread::sleep_for(1s);
        }
    }

private:
    std::mutex mutex_;
    AngleRadians startingAngle_;
    pex::wx::Async<double> currentAngle_;
    pex::model::Signal start_;
    pex::model::Signal stop_;
    std::atomic_bool isRunning_;
    std::thread worker_;
};


template<typename Radians, typename Degrees, typename Signal>
class ExampleFrame: public wxFrame
{
public:
    ExampleFrame(
        RadiansControl<AngleRadians> radiansControl,
        DegreesControl<AngleRadians> degreesControl,
        Radians radians,
        Degrees degrees,
        Signal start,
        Signal stop);
};


// Creates the main function for us, and initializes the app's run loop.
wxshimIMPLEMENT_APP(ExampleApp)


bool ExampleApp::OnInit()
{
    auto asyncControl = this->currentAngle_.GetWxControl();

    auto exampleFrame =
        new ExampleFrame(
            MakeRadiansControl(this->startingAngle_),
            MakeDegreesControl(this->startingAngle_),
            MakeRadiansControl(asyncControl),
            MakeDegreesControl(asyncControl),
            pex::control::Signal<void>(this->start_),
            pex::control::Signal<void>(this->stop_));

    exampleFrame->Show();
    return true;
}


template<typename Radians, typename Degrees, typename Signal>
ExampleFrame<Radians, Degrees, Signal>::ExampleFrame(
    RadiansControl<AngleRadians> radiansControl,
    DegreesControl<AngleRadians> degreesControl,
    Radians radians,
    Degrees degrees,
    Signal start,
    Signal stop)
    :
    wxFrame(nullptr, wxID_ANY, "pex::wx::Field Demo")
{
    using namespace pex::wx;

    auto radiansView =
        LabeledWidget(
            this,
            "Radians:",
            new View(this, radians));

    auto degreesView =
        LabeledWidget(
            this,
            "Degrees:",
            new View(this, degrees));

    auto radiansEntry =
        LabeledWidget(
            this,
            "Radians start:",
            new Field(this, radiansControl));

    auto degreesEntry =
        LabeledWidget(
            this,
            "Degrees start:",
            new Field(this, degreesControl));

    auto startButton = new Button(this, "Start", start);
    auto stopButton = new Button(this, "Stop", stop);

    auto topSizer = std::make_unique<wxBoxSizer>(wxVERTICAL);

    auto fieldsSizer = LayoutLabeled(
        LayoutOptions{},
        radiansView,
        degreesView,
        radiansEntry,
        degreesEntry);

    topSizer->Add(fieldsSizer.release(), 0, wxALL, 10);

    auto buttonSizer = std::make_unique<wxBoxSizer>(wxHORIZONTAL);
    buttonSizer->Add(startButton, 0, wxRIGHT, 5);
    buttonSizer->Add(stopButton);

    topSizer->Add(buttonSizer.release());

    this->SetSizerAndFit(topSizer.release());
}
