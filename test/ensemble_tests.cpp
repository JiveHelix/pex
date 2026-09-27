#include <catch2/catch.hpp>


#include <pex/group.h>
#include "test_observer.h"


// Place types used by this translation unit in a namespace to avoid conflicts
// with other translation units that are part of the catch2 unit tests.
namespace ensemble
{


template<template<typename> typename T>
struct PointSchema
{
    T<double> x;
    T<double> y;

    static constexpr auto fieldsTypeName = "Point";
};


using PointGroup = pex::Group<PointSchema>;


template<template<typename> typename T>
struct CircleSchema
{
    T<PointGroup> center;
    T<double> radius;

    static constexpr auto fieldsTypeName = "Circle";
};


using CircleGroup = pex::Group<CircleSchema>;


template<template<typename> typename T>
struct StuffSchema
{
    T<CircleGroup> leftCircle;
    T<CircleGroup> rightCircle;
    T<PointGroup> aPoint;
    T<double> aLength;

    static constexpr auto fieldsTypeName = "Stuff";
};


using StuffGroup = pex::Group<StuffSchema>;


using Point = typename PointGroup::Plain;
using Circle = typename CircleGroup::Plain;
using Stuff = typename StuffGroup::Plain;


DECLARE_EQUALITY_OPERATORS(Point)
DECLARE_EQUALITY_OPERATORS(Circle)
DECLARE_EQUALITY_OPERATORS(Stuff)


} // end namespace ensemble


template<typename T, typename = void>
struct HasMemberDisconnect_: std::false_type {};

template<typename T>
struct HasMemberDisconnect_
<
    T,
    std::void_t
    <
        decltype(std::declval<T>().Disconnect(NULL))
    >
>: std::true_type {};


template<typename T>
inline constexpr bool HasMemberDisconnect = HasMemberDisconnect_<T>::value;



TEST_CASE("control::Value has member function Disconnect", "[ensemble]")
{
    using Control = pex::control::Value<pex::model::Value<double>>;
    STATIC_REQUIRE(HasMemberDisconnect<Control>);
}


TEST_CASE("ControlTailor has member function Disconnect", "[ensemble]")
{
    using Control = pex::ControlTailor<double>;
    STATIC_REQUIRE(HasMemberDisconnect<Control>);
}

TEST_CASE("EnsembleTailor has member function Disconnect", "[ensemble]")
{
    using Control =
        pex::detail::EnsembleTailor<pex::ControlTailor>
            ::template TailorType<double>;

    static_assert(pex::IsControl<Control>);

    STATIC_REQUIRE(HasMemberDisconnect<Control>);
}


TEST_CASE("Setting Ensemble does not repeat notifications", "[ensemble]")
{
    using Model = typename ensemble::StuffGroup::Model;
    using Control = typename ensemble::StuffGroup::template Control<Model>;
    Model model;
    PEX_ROOT(model);
    Control control(model);

    TestObserver observer(control);

    ensemble::Circle leftCircle{
        {400.0, 800.0},
        42.0};

    ensemble::Circle rightCircle{
        {900.0, 800.0},
        36.0};

    ensemble::Stuff stuff{
        leftCircle,
        rightCircle,
        {42.0, 42.0},
        3.1415926};

    REQUIRE(observer.GetCount() == 0);
    model.Set(stuff);
    REQUIRE(observer.GetCount() == 1);
}


TEST_CASE("Deferred Ensemble does not repeat notifications", "[ensemble]")
{
    using Model = typename ensemble::StuffGroup::Model;
    using Control = typename ensemble::StuffGroup::template Control<Model>;
    Model model;
    PEX_ROOT(model);
    Control control(model);
    TestObserver observer(control);

    ensemble::Circle leftCircle{
        {400.0, 800.0},
        42.0};

    ensemble::Circle rightCircle{
        {900.0, 800.0},
        36.0};

    ensemble::Stuff stuff{
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


TEST_CASE("Deferred member struct does not repeat notifications", "[ensemble]")
{
    using Model = typename ensemble::StuffGroup::Model;
    using Control = typename ensemble::StuffGroup::template Control<Model>;
    Model model;
    PEX_ROOT(model);
    Control control(model);
    TestObserver observer(control);

    ensemble::Stuff stuff{};

    ensemble::Circle rightCircle{
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
