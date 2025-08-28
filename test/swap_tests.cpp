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
    Model model1;
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
