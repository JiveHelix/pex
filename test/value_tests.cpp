/**
  * @author Jive Helix (jivehelix@gmail.com)
  * @copyright 2020 Jive Helix
  * Licensed under the MIT license. See LICENSE file.
  */

#include <catch2/catch.hpp>
#include "jive/testing/cast_limits.h"
#include "jive/testing/gettys_words.h"
#include "pex/value.h"
#include <string>


template<typename T, typename Model>
class Observer
{
public:
    using Interface = pex::interface::Value<Observer<T, Model>, Model>;

    Observer(Model &model)
        :
        interface_(&model),
        observedValue{this->interface_.Get()}
    {
        this->interface_.Connect(this, &Observer::Observe_);
    }

    void Set(typename pex::detail::Argument<T>::Type value)
    {
        this->interface_.Set(value);
    }

private:
    void Observe_(typename pex::detail::Argument<T>::Type value)
    {
        this->observedValue = value;
    }

    Interface interface_;

public:
    T observedValue;
};


TEMPLATE_TEST_CASE(
    "Numeric value propagation",
    "[value]",
    int8_t,
    uint8_t,
    int16_t,
    uint16_t,
    int32_t,
    uint32_t,
    int64_t,
    uint64_t,
    float,
    double,
    long double)
{
    auto original = GENERATE(
        take(
            3, 
            random(
                CastLimits<TestType>::Min(),
                CastLimits<TestType>::Max())));

    auto propagated = GENERATE(
        take(
            10,
            random(
                CastLimits<TestType>::Min(),
                CastLimits<TestType>::Max())));

    using Model = pex::model::Value<TestType>;
    Model model{original};
    Observer<TestType, Model> observer(model);

    if constexpr (std::is_floating_point_v<TestType>)
    {
        REQUIRE(observer.observedValue == original);
    }
    else
    {
        REQUIRE(observer.observedValue == Approx(original));
    }

    model.Set(propagated);

    if constexpr (std::is_floating_point_v<TestType>)
    {
        REQUIRE(observer.observedValue == propagated);
    }
    else
    {
        REQUIRE(observer.observedValue == Approx(propagated));
    }
}


TEST_CASE("std::string propagation", "[value]")
{
    auto wordCount = GENERATE(take(10, random(1u, 10u)));
    
    std::string original = RandomGettysWords().MakeWords(wordCount);
    std::string propagated = RandomGettysWords().MakeWords(wordCount);

    using Model = pex::model::Value<std::string>;
    Model model(original);
    Observer<std::string, Model> observer(model);
    
    REQUIRE(observer.observedValue == original);
    model.Set(propagated);
    REQUIRE(observer.observedValue == propagated);
}


TEMPLATE_TEST_CASE(
    "Numeric value fan-out propagation",
    "[value]",
    int8_t,
    uint8_t,
    int16_t,
    uint16_t,
    int32_t,
    uint32_t,
    int64_t,
    uint64_t,
    float,
    double,
    long double)
{
    auto original = GENERATE(
        take(
            3, 
            random(
                CastLimits<TestType>::Min(),
                CastLimits<TestType>::Max())));

    auto propagated = GENERATE(
        take(
            10,
            random(
                CastLimits<TestType>::Min(),
                CastLimits<TestType>::Max())));

    using Model = pex::model::Value<TestType>;
    Model model{original};
    Observer<TestType, Model> observer1(model);
    Observer<TestType, Model> observer2(model);
    Observer<TestType, Model> observer3(model);
    Observer<TestType, Model> observer4(model);

    if constexpr (std::is_floating_point_v<TestType>)
    {
        REQUIRE(observer1.observedValue == original);
        REQUIRE(observer2.observedValue == original);
        REQUIRE(observer3.observedValue == original);
        REQUIRE(observer4.observedValue == original);
    }
    else
    {
        REQUIRE(observer1.observedValue == Approx(original));
        REQUIRE(observer2.observedValue == Approx(original));
        REQUIRE(observer3.observedValue == Approx(original));
        REQUIRE(observer4.observedValue == Approx(original));
    }

    // Interface values echo back to us, and fan out to all other observers.
    observer1.Set(propagated);

    if constexpr (std::is_floating_point_v<TestType>)
    {
        REQUIRE(observer1.observedValue == propagated);
        REQUIRE(observer2.observedValue == propagated);
        REQUIRE(observer3.observedValue == propagated);
        REQUIRE(observer4.observedValue == propagated);
    }
    else
    {
        REQUIRE(observer1.observedValue == Approx(propagated));
        REQUIRE(observer2.observedValue == Approx(propagated));
        REQUIRE(observer3.observedValue == Approx(propagated));
        REQUIRE(observer4.observedValue == Approx(propagated));
    }
}
