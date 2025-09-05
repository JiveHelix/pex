/**
  * @author Jive Helix (jivehelix@gmail.com)
  * @copyright 2020 Jive Helix
  * Licensed under the MIT license. See LICENSE file.
  */

#include <catch2/catch.hpp>
#include <string>
#include <fields/fields.h>
#include <pex/group.h>
#include "test_observer.h"


// Place types used by this translation unit in a namespace to avoid conflicts
// with other translation units that are part of the catch2 unit tests.
namespace swap
{


template<typename T>
struct PointFields
{
    static constexpr auto fields = std::make_tuple(
        fields::Field(&T::x, "x"),
        fields::Field(&T::y, "y"),
        fields::Field(&T::units, "units"));
};


struct Units
{
    using Type = std::string;

    static std::vector<Type> GetChoices()
    {
        return {"meters", "feet", "furlongs"};
    }
};


template<template<typename> typename T>
struct PointTemplate
{
    T<double> x;
    T<double> y;
    T<pex::MakeSelect<Units>> units;

    static constexpr auto fields = PointFields<PointTemplate<T>>::fields;
    static constexpr auto fieldsTypeName = "Point";
};


using ModelSelectString =
    pex::model::Select<std::string, pex::SelectType<Units>>;

using ControlSelectString = pex::control::Select<ModelSelectString>;


struct PointGroupTemplates_
{
    // Define a customized model
    template<typename GroupBase>
    struct Model: public GroupBase
    {
        static_assert(
            std::is_same_v
            <
                decltype(Model::units),
                ModelSelectString
            >);

        Model()
            :
            GroupBase()
        {

        }

        double GetLength() const
        {
            auto xValue = this->x.Get();
            auto yValue = this->y.Get();
            return std::sqrt(xValue * xValue + yValue * yValue);
        }
    };
};


static_assert(pex::IsMakeSelect<pex::MakeSelect<std::string>>);


using PointGroup = pex::Group<PointFields, PointTemplate, PointGroupTemplates_>;
using PointModel = typename PointGroup::Model;
using PointControl = typename PointGroup::template Control<PointModel>;
using PointMux = typename PointGroup::Mux;


static_assert(
    std::is_same_v
    <
        decltype(PointControl::units),
        ControlSelectString
    >);


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

using Point = typename PointGroup::Plain;
using Circle = typename CircleGroup::Plain;

DECLARE_EQUALITY_OPERATORS(Point)
DECLARE_EQUALITY_OPERATORS(Circle)


} // end namespace swap


TEST_CASE("Can swap control to a different model", "[swap_tests]")
{
    using Model = typename swap::CircleGroup::Model;
    using Follow = pex::FollowSelector<swap::CircleGroup>;
    using Mux = typename swap::CircleGroup::Mux;


    static_assert(
        std::is_same_v
        <
            typename Follow::Upstream,
            Mux
        >);

    using CenterFollow = decltype(Follow::center);

    static_assert(
        std::is_same_v
        <
            typename CenterFollow::Upstream,
            swap::PointMux
        >);

    Model model0;
    PEX_ROOT(model0);

    Model model1;
    PEX_ROOT(model1);

    Mux mux(model0);
    Follow follow(mux);

    TestObserver<Follow> observer(follow);

    model0.center.x = 3;
    model0.center.y = 4;
    model0.radius = 42;

    model1.center.x = -10;
    model1.center.y = -12;
    model1.radius = 25;

    REQUIRE(follow.center.x.Get() == 3);
    REQUIRE(follow.center.y.Get() == 4);
    REQUIRE(follow.radius.Get() == 42);

    REQUIRE(observer.observedValue.center.x == 3);
    REQUIRE(observer.observedValue.center.y == 4);
    REQUIRE(observer.observedValue.radius == 42);

    mux.ChangeUpstream(model1);
    model1.Notify();

    REQUIRE(follow.center.x.Get() == -10);
    REQUIRE(follow.center.y.Get() == -12);
    REQUIRE(follow.radius.Get() == 25);

    REQUIRE(observer.observedValue.center.x == -10);
    REQUIRE(observer.observedValue.center.y == -12);
    REQUIRE(observer.observedValue.radius == 25);
}


namespace swap
{


template<typename T>
struct FooFields
{
    static constexpr auto fields = std::make_tuple(
        fields::Field(&T::name, "name"),
        fields::Field(&T::circles, "circles"));
};


template<template<typename> typename T>
struct FooTemplate
{
    T<std::string> name;
    T<pex::List<CircleGroup>> circles;

    static constexpr auto fields = FooFields<FooTemplate<T>>::fields;
    static constexpr auto fieldsTypeName = "Foo";
};


using FooGroup = pex::Group<FooFields, FooTemplate>;
using FooModel = typename FooGroup::Model;
using FooControl = typename FooGroup::DefaultControl;
using FooMux = typename FooGroup::Mux;
using FooFollow = typename FooGroup::Follow;
using Foo = typename FooGroup::Plain;

DECLARE_EQUALITY_OPERATORS(Foo)


} // end namespace swap


TEST_CASE("Can swap control with list to a different model", "[swap_tests]")
{
    swap::FooModel model0;
    swap::FooModel model1;

    PEX_ROOT(model0);
    PEX_ROOT(model1);

    swap::FooMux mux(model0);
    swap::FooFollow follow(mux);

    TestObserver<swap::FooFollow> observer(follow);

    PEX_ROOT(observer);

    using ModelCircles = decltype(swap::FooModel::circles);
    static_assert(pex::IsListModel<ModelCircles>);

    static_assert(
        pex::IsListNode
        <
            typename pex::List<swap::CircleGroup>::Follow
        >);

    static_assert(
        pex::IsListControl<typename pex::PromoteControl<ModelCircles>::Type>);

    using MuxCircles = decltype(swap::FooMux::circles);
    static_assert(pex::IsListMux<MuxCircles>);

    static_assert(
        pex::IsListFollow<typename pex::PromoteControl<MuxCircles>::Type>);

    static_assert(
        std::is_same_v
        <
            typename pex::PromoteControl<MuxCircles>::Type,
            typename ModelCircles::ListType::Follow
        >);

    TestObserver<swap::FooModel> modelObserver0(model0);
    PEX_ROOT(modelObserver0);
    TestObserver<swap::FooMux> muxObserver(mux);
    PEX_ROOT(muxObserver);

    model0.name = "foo0";
    model1.name = "foo1";

    for (int i = 0; i < 3; ++i)
    {
        model0.circles.Append(swap::Circle{{3.0, 4.0, "feet"}, double(i)});

        model1.circles.Append(
            swap::Circle{{5.0, 6.0, "furlongs"}, double(i + 10)});
    }

    REQUIRE(follow.circles.size() == 3);
    REQUIRE(follow.name.Get() == "foo0");
    REQUIRE(follow.circles.at(1).radius.Get() == Approx(1));
    REQUIRE(follow.circles.at(1).center.units.Get() == "feet");

    REQUIRE(observer.observedValue.circles.size() == 3);
    REQUIRE(observer.observedValue.name == "foo0");
    REQUIRE(observer.observedValue.circles.at(1).radius == Approx(1));
    REQUIRE(observer.observedValue.circles.at(1).center.units == "feet");

    REQUIRE(modelObserver0.observedValue.circles.size() == 3);
    REQUIRE(modelObserver0.observedValue.name == "foo0");
    REQUIRE(modelObserver0.observedValue.circles.at(1).radius == Approx(1));
    REQUIRE(modelObserver0.observedValue.circles.at(1).center.units == "feet");

    REQUIRE(muxObserver.observedValue.circles.size() == 3);
    REQUIRE(muxObserver.observedValue.name == "foo0");
    REQUIRE(muxObserver.observedValue.circles.at(1).radius == Approx(1));
    REQUIRE(muxObserver.observedValue.circles.at(1).center.units == "feet");

    mux.ChangeUpstream(model1);
    model1.Notify();

    REQUIRE(follow.circles.size() == 3);
    REQUIRE(follow.name.Get() == "foo1");
    REQUIRE(follow.circles.at(1).radius.Get() == Approx(11));
    REQUIRE(follow.circles.at(1).center.units.Get() == "furlongs");

    REQUIRE(observer.observedValue.circles.size() == 3);
    REQUIRE(observer.observedValue.name == "foo1");
    REQUIRE(observer.observedValue.circles.at(1).radius == Approx(11));
    REQUIRE(observer.observedValue.circles.at(1).center.units == "furlongs");

    // Check that the list can change size.
    model0.circles.Erase(1);

    mux.ChangeUpstream(model0);
    model0.Notify();

    REQUIRE(follow.circles.size() == 2);
    REQUIRE(follow.name.Get() == "foo0");
    REQUIRE(follow.circles.at(1).radius.Get() == Approx(2));
    REQUIRE(follow.circles.at(1).center.units.Get() == "feet");

    REQUIRE(observer.observedValue.circles.size() == 2);
    REQUIRE(observer.observedValue.name == "foo0");
    REQUIRE(observer.observedValue.circles.at(1).radius == Approx(2));
    REQUIRE(observer.observedValue.circles.at(1).center.units == "feet");
}
