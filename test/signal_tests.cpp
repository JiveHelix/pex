/**
  * @author Jive Helix (jivehelix@gmail.com)
  * @copyright 2020 Jive Helix
  * Licensed under the MIT license. See LICENSE file.
  */

#include <catch2/catch.hpp>
#include "pex/signal.h"
#include "pex/terminus.h"


template<typename Access = pex::GetAndSetTag>
class Observer
{
public:
    static constexpr auto observerName = "signal_tests::Observer";
    using Control = pex::control::Signal<pex::model::Signal, Access>;

    Observer(pex::model::Signal &model)
        :
        terminus_(Control(model)),
        observedCount{0}
    {
        if constexpr (pex::HasAccess<pex::GetTag, Access>)
        {
            PEX_LOG("Connect");
            this->terminus_.Connect(this, &Observer::Observe_);
        }
    }

    void Trigger()
    {
        this->terminus_.Trigger();
    }

private:
    void Observe_()
    {
        ++this->observedCount;
    }

    pex::Terminus<Observer, Control> terminus_;

public:
    int observedCount;
};


TEST_CASE("Signal propagation", "[signal]")
{
    auto signalCount = GENERATE(take(10, random(1, 10000)));
    auto expectedObservedCount = signalCount;
    pex::model::Signal signal;
    PEX_ROOT(signal);
    Observer observer(signal);

    while (signalCount-- > 0)
    {
        signal.Trigger();
    }

    REQUIRE(observer.observedCount == expectedObservedCount);
}


TEST_CASE("Signal fan out", "[signal]")
{
    auto signalCount = GENERATE(take(10, random(1, 10000)));
    auto expectedObservedCount = signalCount;
    pex::model::Signal signal;
    PEX_ROOT(signal);
    Observer observer1(signal);
    Observer observer2(signal);
    Observer observer3(signal);

    // Control signals echo back to us, and fan out to all other observers.
    while (signalCount-- > 0)
    {
        observer1.Trigger();
    }

    REQUIRE(observer1.observedCount == expectedObservedCount);
    REQUIRE(observer2.observedCount == expectedObservedCount);
    REQUIRE(observer3.observedCount == expectedObservedCount);
}


TEST_CASE("Signal fan out from write-only control", "[signal]")
{
    auto signalCount = GENERATE(take(10, random(1, 10000)));
    auto expectedObservedCount = signalCount;
    pex::model::Signal signal;
    PEX_ROOT(signal);
    Observer<pex::SetTag> observer1(signal);
    Observer<pex::GetTag> observer2(signal);
    Observer<pex::GetTag> observer3(signal);

    // Control signals echo back to us, and fan out to all other observers.
    while (signalCount-- > 0)
    {
        observer1.Trigger();
    }

    // observer1 is write-only, so does not receive notifications.
    REQUIRE(observer1.observedCount == 0);
    REQUIRE(observer2.observedCount == expectedObservedCount);
    REQUIRE(observer3.observedCount == expectedObservedCount);
}


TEST_CASE("Signal Terminus is detected", "[signal]")
{
    using ModelSignal = pex::model::Signal;
    using ControlSignal = pex::control::Signal<ModelSignal>;
    using TerminusSignal = pex::Terminus<void, ControlSignal>;
    STATIC_REQUIRE(pex::IsSignalModel<ModelSignal>);
    STATIC_REQUIRE(pex::IsSignalControl<ControlSignal>);
    STATIC_REQUIRE(pex::IsSignal<ModelSignal>);
    STATIC_REQUIRE(pex::IsSignal<ControlSignal>);
    STATIC_REQUIRE(pex::IsSignal<typename TerminusSignal::Upstream>);
}


TEST_CASE(
    "control::Signal does not connect to model without connections",
    "[signal]")
{
    using ModelSignal = pex::model::Signal;
    using ControlSignal = pex::control::Signal<ModelSignal>;

    std::unique_ptr<ControlSignal> control;
    ModelSignal model;
    PEX_ROOT(model);
    control = std::make_unique<ControlSignal>(model);

    REQUIRE(!model.HasObserver(control.get()));
}
