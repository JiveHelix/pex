/**
  * @author Jive Helix (jivehelix@gmail.com)
  * @copyright 2020 Jive Helix
  * Licensed under the MIT license. See LICENSE file.
  */

#include <catch2/catch.hpp>
#include "pex/value.h"
#include "jive/testing/cast_limits.h"
#include "jive/angles.h"


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

private:
    void Observe_(T value)
    {
        this->observedValue = value;
    }

    Interface interface_;

public:
    T observedValue;
};


template<typename T>
struct RangeFilter
{ 
    T Set(T value)
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

    using angles = jive::Angles<TestType>;
    TestType low = -angles::pi;
    TestType high = angles::pi;
    
    using Model =
        pex::model::Value<TestType, RangeFilter<TestType>>;

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
    auto value = GENERATE(
        take(
            30,
            random(
                CastLimits<TestType>::Min(),
                CastLimits<TestType>::Max())));

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
        pex::model::Value<TestType, RangeFilter<TestType>>;
    
    RangeFilter<TestType> filter{low, high};
    Model model{filter};
    Observer<TestType, Model> observer(model);
    model.Set(value);
    REQUIRE(observer.observedValue <= high);
    REQUIRE(observer.observedValue >= low);
}


/** The interface uses degrees, while the model uses radians. **/
template<typename T>
struct DegreesFilter
{
    /** Convert to degrees on retrieval **/
    static T Get(T value)
    {
        return jive::ToDegrees(value);
    }

    /** Convert back to radians on assignment **/
    static T Set(T value)
    {
        return jive::ToRadians(value);
    }
};


TEMPLATE_TEST_CASE(
    "Use interface filter to convert from radians to degrees.",
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
    using Interface =
        pex::interface::Value<void, Model, DegreesFilter<TestType>>;
    
    Model model;
    Interface interface(&model);

    // Set value in degrees.
    interface.Set(value);
    
    using angles = jive::Angles<TestType>;

    // Expect model to be in radians.
    auto expected = angles::pi * value / angles::halfRotationDegrees;
    REQUIRE(model.Get() == Approx(expected));

    // Expect interface to retrieve degrees.
    REQUIRE(interface.Get() == Approx(value));
}
