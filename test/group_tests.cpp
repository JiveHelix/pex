#include <catch2/catch.hpp>
#include <fields/marshal.h>
#include <pex/group.h>
#include <pex/endpoint.h>
#include <pex/select.h>


// Place types used by this translation unit in a namespace to avoid conflicts
// with other translation units that are part of the catch2 unit tests.
namespace groups
{


template<typename T>
struct PointFields
{
    static constexpr auto fields = std::make_tuple(
        fields::Field(&T::x, "x"),
        fields::Field(&T::y, "y"),
        fields::Field(&T::units, "units"));
};


template<template<typename> typename T>
struct PointTemplate
{
    T<double> x;
    T<double> y;
    T<pex::MakeSelect<std::string>> units;

    static constexpr auto fields = PointFields<PointTemplate<T>>::fields;
    static constexpr auto fieldsTypeName = "Point";
};


static_assert(pex::IsMakeSelect<pex::MakeSelect<std::string>>);


using PointGroup = pex::Group<PointFields, PointTemplate>;
using PointControl = typename PointGroup::Control;
using ModelSelectString = pex::model::Select<std::string>;
using ControlSelectString = pex::control::Select<ModelSelectString>;


// Define a customized model
struct PointModel: public PointGroup::Model
{
    static_assert(
        std::is_same_v
        <
            decltype(pex::Group<PointFields, PointTemplate>::Model::units),
            ModelSelectString
        >);

    static_assert(
        std::is_same_v
        <
            decltype(PointModel::units),
            ModelSelectString
        >);

    PointModel()
        :
        PointGroup::Model()
    {
        this->units.SetChoices({"meters", "feet", "furlongs"});
    }

    double GetLength() const
    {
        auto xValue = this->x.Get();
        auto yValue = this->y.Get();
        return std::sqrt(xValue * xValue + yValue * yValue);
    }
};


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
    T<pex::MakeGroup<PointGroup, PointModel>> center;
    T<double> radius;

    static constexpr auto fields = CircleFields<CircleTemplate<T>>::fields;
    static constexpr auto fieldsTypeName = "Circle";
};


using CircleGroup = pex::Group<CircleFields, CircleTemplate>;

using Point = typename PointGroup::Plain;
using Circle = typename CircleGroup::Plain;

DECLARE_EQUALITY_OPERATORS(Point)
DECLARE_EQUALITY_OPERATORS(Circle)


} // end namespace groups


TEST_CASE("Customized model is used", "[groups]")
{
    using Model = typename groups::CircleGroup::Model;
    using DeducedPoint = decltype(Model::center);

    STATIC_REQUIRE(std::is_same_v<DeducedPoint, groups::PointModel>);
}


template<typename Plain, typename ThisIsAControl>
class Observer
{
public:
    static constexpr auto observerName = "groups::Observer";

    Observer(const ThisIsAControl &control)
        :
        control_(control),
        connect_(this, this->control_, &Observer::OnValue),
        observed{}
    {

    }

    void OnValue(const Plain &value)
    {
        this->observed = value;
    }

    ThisIsAControl control_;
    pex::MakeConnector<Observer, ThisIsAControl> connect_;
    Plain observed;
};


TEST_CASE("Terminus aggregate observer receives message.", "[groups]")
{
    using Model = typename groups::CircleGroup::Model;
    using Control = typename groups::CircleGroup::Control;

    using TestObserver = Observer<groups::Circle, groups::CircleGroup::Control>;

    Model model{};

    TestObserver observer{Control(model)};

    model.center.x.Set(10.0);
    model.radius.Set(52.0);

    REQUIRE(model.Get() == observer.observed);
};


TEST_CASE("Terminus aggregate member observer receives message.", "[groups]")
{
    using Model = typename groups::CircleGroup::Model;

    using TestObserver = Observer<groups::Point, groups::PointControl>;

    Model model{};

    TestObserver observer(groups::PointControl(model.center));

    model.center.x.Set(10.0);
    model.center.y.Set(42.0);

    REQUIRE(model.center.Get() == observer.observed);
};


class EndpointObserver
{
public:
    using Control = typename groups::CircleGroup::Control;
    using Endpoints = pex::EndpointGroup<EndpointObserver, Control>;

    using RadiusEndpoint =
        pex::Endpoint<EndpointObserver, decltype(Control::radius)>;

    EndpointObserver(Control control)
        :
        center(),
        radius(),
        endpoints_(this, control)
    {
        this->endpoints_.center.Connect(&EndpointObserver::OnCenter_);
        this->endpoints_.radius.Connect(&EndpointObserver::OnRadius_);
    }

    void OnCenter_(const groups::Point &center_)
    {
        this->center = center_;
    }

    void OnRadius_(double radius_)
    {
        this->radius = radius_;
    }

    groups::Point center;
    double radius;

private:
    Endpoints endpoints_;
    RadiusEndpoint radiusEndpoint_;
};


class RadiusObserver
{
public:
    using Control = typename groups::CircleGroup::Control;

    using RadiusEndpoint =
        pex::EndpointControl<RadiusObserver, decltype(Control::radius)>;

    RadiusObserver()
        :
        endpoint(this)
    {

    }

    RadiusObserver(const Control &control)
        :
        endpoint(this, control.radius, &RadiusObserver::OnRadius_)
    {

    }

    void SetControl(const Control &control)
    {
        this->endpoint.ConnectUpstream(
            control.radius,
            &RadiusObserver::OnRadius_);
    }

    void OnRadius_(double radius_)
    {
        this->radius = radius_;
    }

    double radius;
    RadiusEndpoint endpoint;
};


TEST_CASE("EndpointGroup receives message.", "[groups]")
{
    using Model = typename groups::CircleGroup::Model;
    using Control = typename groups::CircleGroup::Control;

    Model model{};
    Control control(model);

    EndpointObserver endpointObserver{control};

    auto plain = model.Get();
    plain.center.x = 10.0;
    plain.center.y = 42.0;
    plain.center.units = "furlongs";
    plain.radius = 3.1415926;

    control.Set(plain);

    REQUIRE(endpointObserver.center.x == Approx(10.0));
    REQUIRE(endpointObserver.center.y == Approx(42.0));
    REQUIRE(endpointObserver.center.units == "furlongs");
    REQUIRE(endpointObserver.radius == Approx(3.1415926));
};


TEST_CASE("Default constructed single Endpoint receives message.", "[groups]")
{
    using Model = typename groups::CircleGroup::Model;
    using Control = typename groups::CircleGroup::Control;

    Model model{};
    Control control(model);

    RadiusObserver radiusObserver{};
    radiusObserver.SetControl(control); // Control(model));
    control.radius.Set(3.1415926);

    REQUIRE(radiusObserver.radius == Approx(3.1415926));
};


TEST_CASE("Single Endpoint receives message.", "[groups]")
{
    using Model = typename groups::CircleGroup::Model;
    using Control = typename groups::CircleGroup::Control;

    Model model{};
    Control control(model);

    RadiusObserver radiusObserver{control};
    control.radius.Set(3.1415926);

    REQUIRE(radiusObserver.radius == Approx(3.1415926));
};

TEST_CASE(
    "Single Endpoint constructed from model receives message.",
    "[groups]")
{
    using Model = typename groups::CircleGroup::Model;
    using Control = typename groups::CircleGroup::Control;

    Model model{};
    Control control(model);

    RadiusObserver radiusObserver{model};
    control.radius.Set(3.1415926);

    REQUIRE(radiusObserver.radius == Approx(3.1415926));
};


TEST_CASE("Default constructed EndpointControl is set.", "[groups]")
{
    using Model = typename groups::CircleGroup::Model;
    using Control = typename groups::CircleGroup::Control;

    Model model{};
    Control control(model);

    RadiusObserver radiusObserver{};
    radiusObserver.SetControl(control);
    control.radius.Set(3.1415926);

    REQUIRE(radiusObserver.endpoint.control.Get() == Approx(3.1415926));
    REQUIRE(radiusObserver.radius == Approx(3.1415926));
};


TEST_CASE("EndpointControl is set.", "[groups]")
{
    using Model = typename groups::CircleGroup::Model;
    using Control = typename groups::CircleGroup::Control;

    Model model{};
    Control control(model);

    RadiusObserver radiusObserver{model};
    control.radius.Set(3.1415926);

    REQUIRE(radiusObserver.endpoint.control.Get() == Approx(3.1415926));
    REQUIRE(radiusObserver.radius == Approx(3.1415926));
};


TEST_CASE("Setting a group value propagates to model and observer.", "[groups]")
{
    using Model = typename groups::CircleGroup::Model;
    using Control = typename groups::CircleGroup::Control;

    Model model{};

    EndpointObserver endpointObserver{Control(model)};

    model.center.x.Set(10.0);
    model.center.y.Set(42.0);
    model.radius.Set(3.1415926);

    REQUIRE(model.center.Get() == endpointObserver.center);
    REQUIRE(model.radius.Get() == endpointObserver.radius);
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
    T<pex::MakeGroup<groups::CircleGroup>> circle;
    T<pex::MakeSignal> redraw;

    static constexpr auto fields =
        CircleWithSignalFields<CircleWithSignalTemplate<T>>::fields;

    static constexpr auto fieldsTypeName = "CircleWithSignal";
};


using CircleWithSignalGroup =
    pex::Group<CircleWithSignalFields, CircleWithSignalTemplate>;

using CircleWithSignal = typename CircleWithSignalGroup::Plain;

DECLARE_EQUALITY_OPERATORS(CircleWithSignal)


TEST_CASE("Presence of signal allows observation.", "[groups]")
{
    using Model = typename CircleWithSignalGroup::Model;
    using Control = typename CircleWithSignalGroup::Control;

    using TestObserver = Observer<CircleWithSignal, Control>;

    Model model{};

    TestObserver observer{Control(model)};

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
