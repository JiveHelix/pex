/**
  * @author Jive Helix (jivehelix@gmail.com)
  * @copyright 2020 Jive Helix
  * Licensed under the MIT license. See LICENSE file.
  */

#include <catch2/catch.hpp>
#include "pex/signal.h"


template<typename Access = pex::GetAndSetTag>
class Observer
{
public:
    Observer(pex::model::Signal &model)
        :
        control_(model),
        observedCount{0}
    {
        if constexpr (pex::HasAccess<pex::GetTag, Access>)
        {
            this->control_.Connect(this, &Observer::Observe_);
        }
    }

    void Trigger()
    {
        this->control_.Trigger();
    }

private:
    void Observe_()
    {
        ++this->observedCount;
    }

    pex::control::Signal<Observer, Access> control_;

public:
    int observedCount;
};


TEST_CASE("Signal propagation", "[signal]")
{
    auto signalCount = GENERATE(take(10, random(1, 10000)));
    auto expectedObservedCount = signalCount;
    pex::model::Signal signal;
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
    Observer<pex::SetTag> observer1(signal);
    Observer<pex::GetTag> observer2(signal);
    Observer<pex::GetTag> observer3(signal);

    // Control signals echo back to us, and fan out to all other observers.
    while (signalCount-- > 0)
    {
        observer1.Trigger();
    }

    REQUIRE(observer1.observedCount == 0);
    REQUIRE(observer2.observedCount == expectedObservedCount);
    REQUIRE(observer3.observedCount == expectedObservedCount);
}

