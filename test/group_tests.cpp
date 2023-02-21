#include <catch2/catch.hpp>
#include <pex/group.h>
#include <fields/marshal.h>

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


// Define a customized model
struct PointModel: public PointGroup::Model
{
    double GetLength() const
    {
        auto xValue = this->x.Get();
        auto yValue = this->y.Get();
        return std::sqrt(xValue * xValue + yValue * yValue);
    }
};


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
    T<pex::MakeGroup<PointGroup, PointModel>> center;
    T<double> radius;

    static constexpr auto fields = CircleFields<CircleTemplate<T>>::fields;
    static constexpr auto fieldsTypeName = "Circle";
};


using CircleGroup = pex::Group<CircleFields, CircleTemplate>;

using Point = typename PointGroup::Plain;
using Circle = typename CircleGroup::Plain;

DECLARE_COMPARISON_OPERATORS(Point)
DECLARE_COMPARISON_OPERATORS(Circle)


TEST_CASE("Customized model is used", "[groups]")
{
    using Model = typename CircleGroup::Model;
    using DeducedPoint = decltype(Model::center);

    STATIC_REQUIRE(std::is_same_v<DeducedPoint, PointModel>);
}


template<typename Plain, typename Model, template<typename> typename Terminus>
class Observer
{
public:
    static constexpr auto observerName = "group_tests::Observer";

    Observer(Model &model)
        :
        terminus(this, model),
        observed{}
    {
        this->terminus.Connect(&Observer::OnValue);
    }

    void OnValue(const Plain &value)
    {
        this->observed = value;
    }

    Terminus<Observer> terminus;
    Plain observed;
};


TEST_CASE("Terminus aggregate observer receives message.", "[groups]")
{
    using Model = typename CircleGroup::Model;

    using TestObserver =
        Observer<Circle, CircleGroup::Model, CircleGroup::Terminus>;

    Model model{};

    TestObserver observer(model);

    model.center.x.Set(10.0);
    model.radius.Set(52.0);

    REQUIRE(model.Get() == observer.observed);
};


TEST_CASE("Terminus aggregate member observer receives message.", "[groups]")
{
    using Model = typename CircleGroup::Model;

    using TestObserver =
        Observer<Point, PointGroup::Model, PointGroup::Terminus>;

    Model model{};

    TestObserver observer(model.center);

    model.center.x.Set(10.0);
    model.center.y.Set(42.0);

    REQUIRE(model.center.Get() == observer.observed);
};


template<typename T>
struct CircleWithSignalFields
{
    static constexpr auto fields = std::make_tuple(
        fields::Field(&T::circle, "circle"),
        fields::Field(&T::redraw, "redraw"));
};


template<template<typename> typename T>
struct CircleWithSignalTemplate
{
    T<pex::MakeGroup<CircleGroup>> circle;
    T<pex::MakeSignal> redraw;

    static constexpr auto fields =
        CircleWithSignalFields<CircleWithSignalTemplate<T>>::fields;

    static constexpr auto fieldsTypeName = "CircleWithSignal";
};


using CircleWithSignalGroup =
    pex::Group<CircleWithSignalFields, CircleWithSignalTemplate>;

using CircleWithSignal = typename CircleWithSignalGroup::Plain;

DECLARE_COMPARISON_OPERATORS(CircleWithSignal)


TEST_CASE("Presence of signal allows observation.", "[groups]")
{
    using Model = typename CircleWithSignalGroup::Model;

    using TestObserver =
        Observer<CircleWithSignal, Model, CircleWithSignalGroup::Terminus>;

    Model model{};

    TestObserver observer(model);

    model.circle.center.x.Set(10.0);
    model.circle.center.y.Set(42.0);

    REQUIRE(model.Get() == observer.observed);
}


TEST_CASE("Presence of signal allows unstructure/structure.", "[groups]")
{
    using Model = typename CircleWithSignalGroup::Model;
    using Plain = typename CircleWithSignalGroup::Plain;

    Model model{};

    model.circle.center.x.Set(10.0);
    model.circle.center.y.Set(42.0);

    auto unstructured = fields::Unstructure<fields::Marshal>(model.Get());
    auto recovered = fields::Structure<Plain>(unstructured);

    REQUIRE(recovered == model.Get());
}
