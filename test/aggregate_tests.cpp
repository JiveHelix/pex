#include <catch2/catch.hpp>
#include <pex/group.h>
#include "test_observer.h"


template<typename T>
struct PointFields
{
    static constexpr auto fields = std::make_tuple(
        fields::Field(&T::x, "x"),
        fields::Field(&T::y, "y"));
};


template<template<typename> typename T>
struct PointTemplate
{
    T<double> x;
    T<double> y;

    static constexpr auto fields = PointFields<PointTemplate<T>>::fields;
    static constexpr auto fieldsTypeName = "Point";
};


using PointGroup = pex::Group<PointFields, PointTemplate>;


template<typename T>
struct CircleFields
{
    static constexpr auto fields = std::make_tuple(
        fields::Field(&T::center, "center"),
        fields::Field(&T::radius, "radius"));
};


template<template<typename> typename T>
struct CircleTemplate
{
    T<pex::MakeGroup<PointGroup>> center;
    T<double> radius;

    static constexpr auto fields = CircleFields<CircleTemplate<T>>::fields;
    static constexpr auto fieldsTypeName = "Circle";
};


using CircleGroup = pex::Group<CircleFields, CircleTemplate>;


template<typename T>
struct StuffFields
{
    static constexpr auto fields = std::make_tuple(
        fields::Field(&T::leftCircle, "leftCircle"),
        fields::Field(&T::rightCircle, "rightCircle"),
        fields::Field(&T::aPoint, "aPoint"),
        fields::Field(&T::aLength, "aLength"));
};


template<template<typename> typename T>
struct StuffTemplate
{
    T<pex::MakeGroup<CircleGroup>> leftCircle;
    T<pex::MakeGroup<CircleGroup>> rightCircle;
    T<pex::MakeGroup<PointGroup>> aPoint;
    T<double> aLength;

    static constexpr auto fields = StuffFields<StuffTemplate<T>>::fields;
    static constexpr auto fieldsTypeName = "Stuff";
};


using StuffGroup = pex::Group<StuffFields, StuffTemplate>;


using Point = typename PointGroup::Plain;
using Circle = typename CircleGroup::Plain;
using Stuff = typename StuffGroup::Plain;


DECLARE_COMPARISON_OPERATORS(Point)
DECLARE_COMPARISON_OPERATORS(Circle)
DECLARE_COMPARISON_OPERATORS(Stuff)


TEST_CASE("Setting Aggregate does not repeat notifications", "[aggregate]")
{
    using Model = typename StuffGroup::Model;
    using Control = typename StuffGroup::Control<void>;
    Model model;
    Control control(model);

    TestObserver observer(control);

    Circle leftCircle{{
        {{400.0, 800.0}},
        42.0}};

    Circle rightCircle{{
        {{900.0, 800.0}},
        36.0}};

    Stuff stuff{{
        leftCircle,
        rightCircle,
        {{42.0, 42.0}},
        3.1415926}};

    REQUIRE(observer.GetCount() == 0);
    model.Set(stuff);
    REQUIRE(observer.GetCount() == 1);
}


TEST_CASE("Deferred Aggregate does not repeat notifications", "[aggregate]")
{
    using Model = typename StuffGroup::Model;
    using Control = typename StuffGroup::Control<void>;
    Model model;
    Control control(model);

    TestObserver observer(control);

    Circle leftCircle{{
        {{400.0, 800.0}},
        42.0}};

    Circle rightCircle{{
        {{900.0, 800.0}},
        36.0}};

    Stuff stuff{{
        leftCircle,
        rightCircle,
        {{42.0, 42.0}},
        3.1415926}};

    REQUIRE(observer.GetCount() == 0);

    {
        auto defer = pex::MakeDefer(model);
        defer.Set(stuff);
        REQUIRE(observer.GetCount() == 0);
    }

    REQUIRE(observer.GetCount() == 1);
}
