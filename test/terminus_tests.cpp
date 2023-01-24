#include <catch2/catch.hpp>


#include "test_observer.h"
#include "pex/terminus.h"
#include "pex/group.h"


using Model = pex::model::Value<int>;
using Control = pex::control::Value<void, Model>;

template<typename Observer>
using Terminus = pex::Terminus<Observer, Control>;

using Observer = TestObserver<Model, Terminus>;


TEST_CASE("Terminus uses new observer after move.", "[terminus]")
{
    Model value(42);
    Observer first(value);

    REQUIRE(first.observedValue == 42);

    first.Set(43);

    REQUIRE(first.observedValue == 43);

    Observer second(std::move(first));

    REQUIRE(second.observedValue == 43);

    second.Set(44);

    REQUIRE(second.observedValue == 44);

    Observer third(value);
    third = std::move(second);

    REQUIRE(third.observedValue == 44);

    third.Set(45);

    REQUIRE(third.observedValue == 45);
}

TEST_CASE("Terminus uses new observer after copy.", "[terminus]")
{
    Model value(42);
    Observer first(value);

    REQUIRE(first.observedValue == 42);

    first.Set(43);

    REQUIRE(first.observedValue == 43);
    
    Observer second(first);

    REQUIRE(second.observedValue == 43);

    second.Set(44);

    REQUIRE(second.observedValue == 44);

    Observer third(value);
    third = second;

    REQUIRE(third.observedValue == 44);

    third.Set(45);

    REQUIRE(third.observedValue == 45);
}


template<typename T>
struct TestFields
{
    static constexpr auto fields = std::make_tuple(
        fields::Field(&T::one, "one"),
        fields::Field(&T::one, "two"),
        fields::Field(&T::one, "three"));
};

template<template<typename> typename T>
struct TestTemplate
{
    T<int> one;
    T<long> two;
    T<double> three;

    static constexpr auto fields = TestFields<TestTemplate>::fields;
};

using TerminusTestGroup = pex::Group<TestFields, TestTemplate>;

using TerminusTestPlain = TerminusTestGroup::Plain;

DECLARE_OUTPUT_STREAM_OPERATOR(TerminusTestPlain)
DECLARE_COMPARISON_OPERATORS(TerminusTestPlain)

using TerminusTestModel = TerminusTestGroup::Model;

template<typename Observer>
using TestTerminus = TerminusTestGroup::Terminus<Observer>;

using TerminusGroupObserver = TestObserver<TerminusTestModel, TestTerminus>;


TEST_CASE("Terminus group uses new observer after move.", "[terminus]")
{
    TerminusTestPlain values{{42, 43, 44.0}};

    TerminusTestModel model(values);
    TerminusGroupObserver first(model);

    REQUIRE(first.observedValue == values);

    values.one = 43;
    first.Set(values);

    REQUIRE(first.observedValue == values);

    TerminusGroupObserver second(std::move(first));

    REQUIRE(second.observedValue == values);

    values.two = 99;
    second.Set(values);

    REQUIRE(second.observedValue == values);

    TerminusGroupObserver third(model);
    third = std::move(second);

    REQUIRE(third.observedValue == values);

    values.three = 45.0;
    third.Set(values);

    REQUIRE(third.observedValue == values);
}


TEST_CASE("Terminus group uses new observer after copy.", "[terminus]")
{
    TerminusTestPlain values{{42, 43, 44.0}};

    TerminusTestModel model(values);
    TerminusGroupObserver first(model);

    REQUIRE(first.observedValue == values);

    values.one = 43;
    first.Set(values);

    REQUIRE(first.observedValue == values);
    
    TerminusGroupObserver second(first);

    REQUIRE(second.observedValue == values);

    values.two = 99;
    second.Set(values);

    REQUIRE(second.observedValue == values);

    TerminusGroupObserver third(model);
    third = second;

    REQUIRE(third.observedValue == values);

    values.three = 45.0;
    third.Set(values);

    REQUIRE(third.observedValue == values);
}

