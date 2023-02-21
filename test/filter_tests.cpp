/**
  * @author Jive Helix (jivehelix@gmail.com)
  * @copyright 2020 Jive Helix
  * Licensed under the MIT license. See LICENSE file.
  */

#ifdef _WIN32
#define _USE_MATH_DEFINES
#endif

#include <cmath>
#include <catch2/catch.hpp>
#include <jive/testing/generator_limits.h>
#include "pex/value.h"


template<typename T, typename Model>
class Observer
{
public:
    static constexpr auto observerName = "filter_tests::Observer";

    using Control =
        pex::control::Value<Observer<T, Model>, Model>;

    Observer(Model &model)
        :
        terminus_(this, model),
        observedValue{this->terminus_.Get()}
    {
        PEX_LOG("Connect");
        this->terminus_.Connect(&Observer::Observe_);
    }

private:
    void Observe_(T value)
    {
        this->observedValue = value;
    }

    pex::Terminus<Observer, Control> terminus_;

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

    TestType low = -static_cast<TestType>(M_PI);
    TestType high = static_cast<TestType>(M_PI);

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
                    GeneratorLimits<TestType>::Lowest(),
                    GeneratorLimits<TestType>::Max()))));

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
        return 180 * value / static_cast<T>(M_PI);
    }

    /** Convert back to radians on assignment **/
    static T Set(T value)
    {
        return static_cast<T>(M_PI) * value / static_cast<T>(180.0);
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

    // Expect model to be in radians.
    auto expected = static_cast<TestType>(M_PI) * value / 180;
    REQUIRE(model.Get() == Approx(expected));

    // Expect control to retrieve degrees.
    REQUIRE(control.Get() == Approx(value));
}
