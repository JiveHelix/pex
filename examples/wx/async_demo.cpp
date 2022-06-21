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
#include <fields/fields.h>

#include "pex/group.h"
#include "pex/value.h"
#include "pex/signal.h"
#include "pex/wx/wxshim.h"
#include "pex/wx/view.h"
#include "pex/wx/field.h"
#include "pex/wx/labeled_widget.h"
#include "pex/wx/async.h"
#include "pex/wx/button.h"


template<typename T>
struct DemoFields
{
    static constexpr auto fields = std::make_tuple(
        fields::Field(&T::startingAngle, "startingAngle"),
        fields::Field(&T::currentAngle, "currentAngle"),
        fields::Field(&T::start, "start"),
        fields::Field(&T::stop, "stop"));
};


template<template<typename> typename T>
struct DemoTemplate
{
    T<double> startingAngle;
    T<pex::wx::MakeAsync<double>> currentAngle;
    T<pex::MakeSignal> start;
    T<pex::MakeSignal> stop;

    static constexpr auto fields = DemoFields<DemoTemplate>::fields;
    static constexpr auto fieldsTypeName = "Demo";
};


using DemoGroup = pex::Group<DemoFields, DemoTemplate>;
using DemoControl = typename DemoGroup::Control<void>;
using DemoModel = typename DemoGroup::Model;


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
        model_{},
        isRunning_{},
        worker_{}
    {
        this->model_.startingAngle.Connect(this, &ExampleApp::OnUpdate_);
        this->model_.start.Connect(this, &ExampleApp::OnStart_);
        this->model_.stop.Connect(this, &ExampleApp::OnStop_);
    }

    bool OnInit() override;

private:
    static void OnUpdate_(void *context, double value)
    {
        auto self = reinterpret_cast<ExampleApp *>(context);
        auto control = self->model_.currentAngle.GetWxControl();
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

        std::cout << "Stopped:" << std::endl;
        std::cout << fields::DescribeColorized(self->model_.Get()) << std::endl;
    }

    void WorkerThread_()
    {
        auto workerControl = this->model_.currentAngle.GetWorkerControl();

        while (this->isRunning_)
        {
            auto next =
                this->model_.currentAngle.Get() + tau::Angles<double>::pi / 4.0;

            workerControl.Set(next);

            using namespace std::chrono_literals;
            std::this_thread::sleep_for(250ms);
        }
    }

private:
    std::mutex mutex_;
    DemoModel model_;
    std::atomic_bool isRunning_;
    std::thread worker_;
};


class ExampleFrame: public wxFrame
{
public:
    ExampleFrame(DemoControl demoControl);
};


// Creates the main function for us, and initializes the app's run loop.
wxshimIMPLEMENT_APP(ExampleApp)


bool ExampleApp::OnInit()
{
    auto exampleFrame = new ExampleFrame(DemoControl(this->model_));

    exampleFrame->Show();

    return true;
}


ExampleFrame::ExampleFrame(DemoControl demoControl)
    :
    wxFrame(nullptr, wxID_ANY, "pex::wx::Field Demo")
{
    using namespace pex::wx;

    auto radiansView =
        LabeledWidget(
            this,
            "Radians:",
            new View(this, demoControl.currentAngle));

    auto degreesView =
        LabeledWidget(
            this,
            "Degrees:",
            new View(this, MakeDegreesControl(demoControl.currentAngle)));

    auto radiansEntry =
        LabeledWidget(
            this,
            "Radians start:",
            new Field(this, demoControl.startingAngle));

    auto degreesEntry =
        LabeledWidget(
            this,
            "Degrees start:",
            new Field(this, MakeDegreesControl(demoControl.startingAngle)));

    auto startButton = new Button(this, "Start", demoControl.start);
    auto stopButton = new Button(this, "Stop", demoControl.stop);

    auto sizer = std::make_unique<wxBoxSizer>(wxVERTICAL);

    auto fieldsSizer = LayoutLabeled(
        LayoutOptions{},
        radiansView,
        degreesView,
        radiansEntry,
        degreesEntry);

    sizer->Add(fieldsSizer.release(), 0, wxALL, 10);

    auto buttonSizer = std::make_unique<wxBoxSizer>(wxHORIZONTAL);
    buttonSizer->Add(startButton, 0, wxRIGHT, 5);
    buttonSizer->Add(stopButton);

    sizer->Add(buttonSizer.release(), 0, wxTOP, 5);

    auto topSizer = std::make_unique<wxBoxSizer>(wxVERTICAL);
    topSizer->Add(sizer.release(), 0, wxALL, 10);
    this->SetSizerAndFit(topSizer.release());
}
