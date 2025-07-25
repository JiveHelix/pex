#include <catch2/catch.hpp>


#include <pex/group.h>
#include "test_observer.h"


// Place types used by this translation unit in a namespace to avoid conflicts
// with other translation units that are part of the catch2 unit tests.
namespace aggregate
{


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
    T<PointGroup> center;
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
    T<CircleGroup> leftCircle;
    T<CircleGroup> rightCircle;
    T<PointGroup> aPoint;
    T<double> aLength;

    static constexpr auto fields = StuffFields<StuffTemplate<T>>::fields;
    static constexpr auto fieldsTypeName = "Stuff";
};


using StuffGroup = pex::Group<StuffFields, StuffTemplate>;


using Point = typename PointGroup::Plain;
using Circle = typename CircleGroup::Plain;
using Stuff = typename StuffGroup::Plain;


DECLARE_EQUALITY_OPERATORS(Point)
DECLARE_EQUALITY_OPERATORS(Circle)
DECLARE_EQUALITY_OPERATORS(Stuff)


} // end namespace aggregate


TEST_CASE("Setting Aggregate does not repeat notifications", "[aggregate]")
{
    using Model = typename aggregate::StuffGroup::Model;
    using Control = typename aggregate::StuffGroup::Control;
    Model model;
    REGISTER_IDENTITY(model);
    Control control(model);

    TestObserver observer(control);

    aggregate::Circle leftCircle{
        {400.0, 800.0},
        42.0};

    aggregate::Circle rightCircle{
        {900.0, 800.0},
        36.0};

    aggregate::Stuff stuff{
        leftCircle,
        rightCircle,
        {42.0, 42.0},
        3.1415926};

    REQUIRE(observer.GetCount() == 0);
    model.Set(stuff);
    REQUIRE(observer.GetCount() == 1);
}


TEST_CASE("Deferred Aggregate does not repeat notifications", "[aggregate]")
{
    using Model = typename aggregate::StuffGroup::Model;
    using Control = typename aggregate::StuffGroup::Control;
    Model model;
    REGISTER_IDENTITY(model);
    Control control(model);
    TestObserver observer(control);

    aggregate::Circle leftCircle{
        {400.0, 800.0},
        42.0};

    aggregate::Circle rightCircle{
        {900.0, 800.0},
        36.0};

    aggregate::Stuff stuff{
        leftCircle,
        rightCircle,
        {42.0, 42.0},
        3.1415926};

    REQUIRE(observer.GetCount() == 0);

    {
        auto defer = pex::MakeDefer(model);
        defer.Set(stuff);
        REQUIRE(observer.GetCount() == 0);
    }

    REQUIRE(observer.observedValue == stuff);
    REQUIRE(observer.GetCount() == 1);
}


TEST_CASE("Deferred member struct does not repeat notifications", "[aggregate]")
{
    using Model = typename aggregate::StuffGroup::Model;
    using Control = typename aggregate::StuffGroup::Control;
    Model model;
    REGISTER_IDENTITY(model);
    Control control(model);
    TestObserver observer(control);

    aggregate::Stuff stuff{};

    aggregate::Circle rightCircle{
        {900.0, 800.0},
        36.0};

    stuff.rightCircle = rightCircle;

    REQUIRE(observer.GetCount() == 0);

    {
        auto defer = pex::MakeDefer(control.rightCircle);
        defer.radius.Set(36.0);
        defer.center.x.Set(900.0);
        defer.center.y.Set(800.0);
        REQUIRE(observer.GetCount() == 0);
    }

    REQUIRE(observer.observedValue == stuff);
    REQUIRE(observer.GetCount() == 1);
}
