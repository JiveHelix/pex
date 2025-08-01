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
#include <fields/fields.h>
#include "pex/value.h"
#include "pex/group.h"
#include "pex/converting_filter.h"
#include "pex/range.h"


template<typename Control>
class Observer
{
public:
    using Type = typename Control::Type;
    static constexpr auto observerName = "filter_tests::Observer";

    Observer(const Control &control)
        :
        terminus_(this, control),
        observedValue{this->terminus_.Get()}
    {
        PEX_LOG("Connect");
        this->terminus_.Connect(&Observer::Observe_);
    }

    Observer(Control &&control)
        :
        terminus_(this, std::move(control)),
        observedValue{this->terminus_.Get()}
    {
        PEX_LOG("Connect");
        this->terminus_.Connect(&Observer::Observe_);
    }

private:
    void Observe_(pex::Argument<Type> value)
    {
        this->observedValue = value;
    }

    pex::Terminus<Observer, Control> terminus_;

public:
    Type observedValue;
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
    using Control = pex::control::Value<Model>;

    Model model{filter};
    PEX_ROOT(model);

    Observer<Control> observer((Control(model)));

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
    PEX_ROOT(model);
    using Control = pex::control::Value<Model>;
    Observer<Control> observer((Control(model)));
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
        pex::control::FilteredValue<Model, Filter>;

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


template<typename T>
struct CoffeeFields
{
    static constexpr auto fields = std::make_tuple(
        fields::Field(&T::id, "id"),
        fields::Field(&T::price, "price"));
};

template<template<typename> typename T>
struct CoffeeTemplate
{
    T<pex::ReadOnly<size_t>> id;
    T<double> price;

    static constexpr auto fields = CoffeeFields<CoffeeTemplate>::fields;
    static constexpr auto fieldsTypeName = "Coffee";
};


TEST_CASE("Use ReadOnly interface to create read-only control", "[filters]")
{
    using Group = pex::Group<CoffeeFields, CoffeeTemplate>;
    using Model = typename Group::Model;
    using Control = typename Group::Control;

    Model model{};
    Control control(model);
    pex::SetOverride(model.id, 42u);

    REQUIRE(control.id.Get() == 42);
}


TEST_CASE(
    "Assign all aggregate members except any read-only members.",
    "[filters]")
{
    using Group = pex::Group<CoffeeFields, CoffeeTemplate>;
    using Model = typename Group::Model;
    using Control = typename Group::Control;
    using Coffee = typename Group::Plain;

    Model model{{0, 11.99}};
    Control control(model);
    Coffee coffee{42, 12.99};

    model.Set(coffee);

    // Aggregate set should have skipped setting the id.
    REQUIRE(model.id.Get() == 0);

    // Set the id.
    pex::SetOverride(model.id, 42u);
    REQUIRE(model.id.Get() == 42);

    REQUIRE(model.price.Get() == 12.99);

    // Models always have Set access.
    REQUIRE(model.id.Get() == 42);

    coffee.id = 13;
    coffee.price = 13.99;
    control.Set(coffee);

    REQUIRE(model.price.Get() == 13.99);

    // id (read-only from the Control) should not have been changed.
    REQUIRE(model.id.Get() == 42);
}


TEST_CASE("Observe a control that follows another control", "[filters]")
{
    using Model = pex::model::Value<double>;
    using Control = pex::control::Value<Model>;
    using Follower = pex::control::Value<Control>;

    Model model;
    PEX_ROOT(model);
    Control control(model);

    Observer<Control> controlObserver{control};
    Observer<Follower> observer{Follower(control)};

    model.Set(3.14);

    REQUIRE(control.HasModel());
    REQUIRE(Follower(control).HasModel());
    REQUIRE(controlObserver.observedValue == 3.14);
    REQUIRE(observer.observedValue == 3.14);
}


TEST_CASE("Observe filtered value", "[filters]")
{
    using Model = pex::model::Value<double>;
    using Control = pex::control::Value<Model>;

    using FilteredControl = pex::control::FilteredValue
    <
        Control,
        pex::control::LinearFilter<double>,
        pex::GetAndSetTag
    >;

    Model model;
    PEX_ROOT(model);
    Control control(model);

    auto observer = Observer(
        FilteredControl(control, pex::control::LinearFilter<double>(1000)));

    model.Set(3.14);

    REQUIRE(observer.observedValue == 3140);
}


TEST_CASE("LinearRange is observable", "[filters]")
{
    using WeightRange = pex::model::Range<double>;
    using WeightControl = pex::control::Range<WeightRange>;
    using FilteredWeight = pex::control::LinearRange<WeightControl>;

    WeightRange weightRange;
    PEX_ROOT(weightRange);
    weightRange.SetMinimum(100.0);
    weightRange.SetMaximum(150.0);

    WeightControl control(weightRange);

    auto observer = Observer(FilteredWeight(control).value);

    control.value.Set(125.0);

    REQUIRE(weightRange.Get() == 125);
    REQUIRE(observer.observedValue == 125);
}


TEST_CASE("Optional LinearRange is observable", "[filters]")
{
    using WeightRange = pex::model::Range<std::optional<double>>;
    using WeightControl = pex::control::Range<WeightRange>;
    using FilteredWeight = pex::control::LinearRange<WeightControl>;
    using FilteredValue = typename FilteredWeight::Value;

    WeightRange weightRange;
    PEX_ROOT(weightRange);
    weightRange.SetMinimum(100.0);
    weightRange.SetMaximum(150.0);

    WeightControl control(weightRange);
    REQUIRE(!weightRange.Get());

    using FilteredCopyable = pex::control::Value<FilteredValue>;

    FilteredWeight filteredWeight(control);

    auto observer = Observer(FilteredCopyable(filteredWeight.value));
    control.value.Set(125.0);

    REQUIRE(observer.observedValue.has_value());
    REQUIRE(*observer.observedValue == 125);

    control.value.Set({});

    REQUIRE(!observer.observedValue);
}


TEST_CASE("Optional ConvertingRange is observable", "[filters]")
{
    RESET_PEX_NAMES

    using WeightRange = pex::model::Range<std::optional<double>>;
    using WeightControl = pex::control::Range<WeightRange>;
    using FilteredWeight = pex::control::ConvertingRange<WeightControl, int>;

    WeightRange weightRange;
    PEX_ROOT(weightRange);
    weightRange.SetMinimum(100.0);
    weightRange.SetMaximum(150.0);

    WeightControl control(weightRange);
    REQUIRE(!weightRange.Get());

    auto observer = Observer(FilteredWeight(control).value);
    control.value.Set(125.0);

    REQUIRE(observer.observedValue.has_value());
    REQUIRE(*observer.observedValue == 125);

    control.value.Set({});

    REQUIRE(!observer.observedValue);
}


TEST_CASE("LogarithmicFilter uses divisor.", "[filters]")
{
    using Filter = pex::control::LogarithmicFilter<double, 2, 3>;

    for (int value = 0; value < 10; ++value)
    {
        REQUIRE(Filter::Get(Filter::Set(value)) == value);
    }
}
