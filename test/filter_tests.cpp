/**
  * @author Jive Helix (jivehelix@gmail.com)
  * @copyright 2020 Jive Helix
  * Licensed under the MIT license. See LICENSE file.
  */

#include <catch2/catch.hpp>
#include <jive/testing/cast_limits.h>
#include <tau/angles.h>
#include "pex/value.h"


template<typename T, typename Model>
class Observer
{
public:
    using Control =
        pex::control::Value<Observer<T, Model>, Model>;

    Observer(Model &model)
        :
        control_(this, model),
        observedValue{this->control_.Get()}
    {
        PEX_LOG("Connect");
        this->control_.Connect(&Observer::Observe_);
    }

private:
    void Observe_(T value)
    {
        this->observedValue = value;
    }

    pex::Terminus<Observer, Control> control_;

public:
    T observedValue;
};


template<typename T>
struct RangeFilter
{
    T Set(T value) const
    {
        return std::max(this->low, std::min(value, this->high));
    }

    T low;
    T high;
};




TEMPLATE_TEST_CASE(
    "Use model filter to limit floating-point value to a range",
    "[filters]",
    float,
    double,
    long double)
{
    auto value = GENERATE(
        take(
            30,
            random(
                static_cast<TestType>(-12.0),
                static_cast<TestType>(12.0))));

    using angles = tau::Angles<TestType>;
    TestType low = -angles::pi;
    TestType high = angles::pi;

    using Model =
        pex::model::FilteredValue<TestType, RangeFilter<TestType>>;

    RangeFilter<TestType> filter{low, high};
    Model model{filter};
    Observer<TestType, Model> observer(model);
    model.Set(value);
    REQUIRE(observer.observedValue <= high);
    REQUIRE(observer.observedValue >= low);
}


TEMPLATE_TEST_CASE(
    "Use model filter to limit integral value to a range",
    "[filters]",
    int8_t,
    uint8_t,
    int16_t,
    uint16_t,
    int32_t,
    uint32_t,
    int64_t,
    uint64_t)
{
    auto value = static_cast<TestType>(
        GENERATE(
            take(
                30,
                random(
                    CastLimits<TestType>::Min(),
                    CastLimits<TestType>::Max()))));

    TestType low;

    if constexpr (std::is_unsigned_v<TestType>)
    {
        low = 13;
    }
    else
    {
        low = -42;
    }

    TestType high = 96;

    using Model =
        pex::model::FilteredValue<TestType, RangeFilter<TestType>>;

    RangeFilter<TestType> filter{low, high};
    Model model{filter};
    Observer<TestType, Model> observer(model);
    model.Set(value);
    REQUIRE(observer.observedValue <= high);
    REQUIRE(observer.observedValue >= low);
}


/** The control uses degrees, while the model uses radians. **/
template<typename T>
struct DegreesFilter
{
    /** Convert to degrees on retrieval **/
    static T Get(T value)
    {
        return tau::ToDegrees(value);
    }

    /** Convert back to radians on assignment **/
    static T Set(T value)
    {
        return tau::ToRadians(value);
    }
};


TEMPLATE_TEST_CASE(
    "Use filter to convert from radians to degrees.",
    "[filters]",
    float,
    double,
    long double)
{
    auto value = GENERATE(
        take(
            30,
            random(
                static_cast<TestType>(-720.0),
                static_cast<TestType>(720.0))));

    using Model = pex::model::Value<TestType>;
    using Filter = DegreesFilter<TestType>;

    using Control =
        pex::control::FilteredValue<void, Model, Filter>;

    Model model;
    Control control(model);

    // Set value in degrees.
    control.Set(value);

    using angles = tau::Angles<TestType>;

    // Expect model to be in radians.
    auto expected = angles::pi * value / angles::piDegrees;
    REQUIRE(model.Get() == Approx(expected));

    // Expect control to retrieve degrees.
    REQUIRE(control.Get() == Approx(value));
}
