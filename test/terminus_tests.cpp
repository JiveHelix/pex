#include <catch2/catch.hpp>

// #define ENABLE_PEX_LOG

#include "test_observer.h"
#include "pex/terminus.h"
#include "pex/group.h"


using Model = pex::model::Value<int>;
using Control = pex::control::Value<Model>;

template<typename Observer>
using Terminus = pex::Terminus<Observer, Control>;

using Observer = TerminusObserver<Control, Terminus>;


TEST_CASE("Terminus uses new observer after move.", "[terminus]")
{
    Model value(42);
    PEX_ROOT(value);
    Control control(value);

    Observer first(control);

    REQUIRE(first.observedValue == 42);

    first.Set(43);

    REQUIRE(first.observedValue == 43);

    Observer second(std::move(first));

    REQUIRE(second.observedValue == 43);

    second.Set(44);

    REQUIRE(second.observedValue == 44);

    Observer third(control);
    third = std::move(second);

    REQUIRE(third.observedValue == 44);

    third.Set(45);

    REQUIRE(third.observedValue == 45);
}

TEST_CASE("Terminus uses new observer after copy.", "[terminus]")
{
    Model value(42);
    PEX_ROOT(value);
    Control control(value);

    Observer first(control);

    REQUIRE(first.observedValue == 42);

    first.Set(43);

    REQUIRE(first.observedValue == 43);

    Observer second(first);

    REQUIRE(second.observedValue == 43);

    second.Set(44);

    REQUIRE(second.observedValue == 44);

    Observer third(control);
    third = second;

    REQUIRE(third.observedValue == 44);

    third.Set(45);

    REQUIRE(third.observedValue == 45);
}


template<template<typename> typename T>
struct TestSchema
{
    T<int> one;
    T<long> two;
    T<double> three;

    static constexpr auto fieldsTypeName = "Test";
};

using TerminusTestGroup = pex::Group<TestSchema>;

using TerminusTestPlain = TerminusTestGroup::Plain;

DECLARE_OUTPUT_STREAM_OPERATOR(TerminusTestPlain)
DECLARE_EQUALITY_OPERATORS(TerminusTestPlain)

using TerminusTestModel = TerminusTestGroup::Model;
using TerminusGroupObserver = TestObserver<TerminusTestModel>;

using GroupControl =
    typename TerminusTestGroup::template Control<TerminusTestModel>;

using EnsembleObserver = TestObserver<GroupControl>;

// This tests that GroupControl can be passed by copy, then used to create
// a Terminus.
std::unique_ptr<EnsembleObserver> MakeTestObserver(GroupControl control)
{
    return std::make_unique<EnsembleObserver>(control);
}


TEST_CASE("pex::Terminus can use Group::Control as its upstream.", "[terminus]")
{
    STATIC_REQUIRE(!pex::IsModel<GroupControl>);
    STATIC_REQUIRE(!pex::IsSignalModel<GroupControl>);

    STATIC_REQUIRE(
        !pex::detail::FilterIsMember
        <
            GroupControl::UpstreamType,
            GroupControl::Filter
        >);

    STATIC_REQUIRE(pex::IsCopyable<GroupControl>);

    TerminusTestPlain values{42, 43, 44.0};
    TerminusTestModel model(values);

    auto observer = MakeTestObserver(GroupControl(model));

    model.one.Set(49);

    TerminusTestPlain expected{49, 43, 44.0};
    REQUIRE(expected == observer->observedValue);
}
