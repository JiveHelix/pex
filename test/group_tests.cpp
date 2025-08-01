#include <catch2/catch.hpp>
#include <fields/marshal.h>
#include <pex/group.h>
#include <pex/endpoint.h>
#include <pex/select.h>
#include "test_observer.h"


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
using PointControl = typename PointGroup::Control;


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
}


TEST_CASE("Terminus aggregate member observer receives message.", "[groups]")
{
    using Model = typename groups::CircleGroup::Model;

    using TestObserver = Observer<groups::Point, groups::PointControl>;

    Model model{};

    TestObserver observer(groups::PointControl(model.center));

    model.center.x.Set(10.0);
    model.center.y.Set(42.0);

    REQUIRE(model.center.Get() == observer.observed);
}


class EndpointObserver: Separator
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
        endpoints_(PEX_THIS("EndpointObserver"), control)
    {
        PEX_MEMBER(center);
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
        pex::Endpoint<RadiusObserver, decltype(Control::radius)>;

    RadiusObserver()
        :
        endpoint(PEX_THIS("RadiusObserver"))
    {
        PEX_MEMBER(endpoint);
    }

    RadiusObserver(const Control &control)
        :
        endpoint(
            PEX_THIS("RadiusObserver"),
            control.radius,
            &RadiusObserver::OnRadius_)
    {
        PEX_MEMBER(endpoint);
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
}


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
}


TEST_CASE("Single Endpoint receives message.", "[groups]")
{
    using Model = typename groups::CircleGroup::Model;
    using Control = typename groups::CircleGroup::Control;

    Model model{};
    Control control(model);

    RadiusObserver radiusObserver{control};
    control.radius.Set(3.1415926);

    REQUIRE(radiusObserver.radius == Approx(3.1415926));
}

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
}


TEST_CASE("Default constructed Endpoint is set.", "[groups]")
{
    using Model = typename groups::CircleGroup::Model;
    using Control = typename groups::CircleGroup::Control;

    Model model{};
    Control control(model);

    RadiusObserver radiusObserver{};
    radiusObserver.SetControl(control);
    control.radius.Set(3.1415926);

    REQUIRE(radiusObserver.endpoint.Get() == Approx(3.1415926));
    REQUIRE(radiusObserver.radius == Approx(3.1415926));
}


TEST_CASE("Endpoint is set.", "[groups]")
{
    using Model = typename groups::CircleGroup::Model;
    using Control = typename groups::CircleGroup::Control;

    Model model{};
    Control control(model);

    RadiusObserver radiusObserver{model};
    control.radius.Set(3.1415926);

    REQUIRE(radiusObserver.endpoint.Get() == Approx(3.1415926));
    REQUIRE(radiusObserver.radius == Approx(3.1415926));
}


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
}


template<typename Object>
class CenterObserver
{
public:
    static constexpr auto observerName = "CenterObserver";

    using Type = typename Object::Type;

    CenterObserver(Object &object)
        :
        connect_(this, object, &CenterObserver::Observe_),
        count_(0),
        observedValue{object.Get()}
    {

    }

    void Set(pex::Argument<Type> value)
    {
        this->connect_.Set(value);
    }

    CenterObserver(CenterObserver &&) = delete;
    CenterObserver & operator=(CenterObserver &&) = delete;
    CenterObserver(const CenterObserver &) = delete;
    CenterObserver & operator=(const CenterObserver &) = delete;

    size_t GetCount() const
    {
        return this->count_;
    }

private:
    void Observe_(pex::Argument<Type> value)
    {
        this->observedValue = value;
        ++this->count_;
    }

    pex::MakeConnector<CenterObserver<Object>, Object> connect_;
    size_t count_;

public:
    Type observedValue;
};


TEST_CASE(
    "Deferring a group only notifies members that were changed.", "[groups]")
{
    using Model = typename groups::CircleGroup::Model;
    using Control = typename groups::CircleGroup::Control;

    Model model{};
    Control control(model);

    CenterObserver centerObserver(control.center);
    TestObserver circleObserver(control);

    {
        auto defer = pex::MakeDefer(control);
        defer.radius.Set(3.1415926);

        REQUIRE(centerObserver.GetCount() == 0);
        REQUIRE(circleObserver.GetCount() == 0);
    }

    REQUIRE(centerObserver.GetCount() == 0);
    REQUIRE(circleObserver.GetCount() == 1);

    REQUIRE(model.radius.Get() == circleObserver.observedValue.radius);
    REQUIRE(model.center.Get() == centerObserver.observedValue);
}


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
    T<groups::CircleGroup> circle;
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


namespace subgroup
{


template<typename T>
struct ColorFields
{
    static constexpr auto fields = std::make_tuple(
        fields::Field(&T::red, "red"),
        fields::Field(&T::green, "green"),
        fields::Field(&T::blue, "blue"));
};


template<template<typename> typename T>
struct ColorTemplate
{
    T<int> red;
    T<int> green;
    T<int> blue;

    static constexpr auto fields = ColorFields<ColorTemplate>::fields;
    static constexpr auto fieldsTypeName = "Color";
};


struct Color: public ColorTemplate<pex::Identity>
{
    Color()
        :
        ColorTemplate<pex::Identity>
        {
            1,
            2,
            3
        }
    {

    }
};


struct ColorCustom
{
    using Plain = Color;
};


using ColorGroup = pex::Group<ColorFields, ColorTemplate, ColorCustom>;
using ColorModel = typename ColorGroup::Model;


template<typename T>
struct PixelFields
{
    static constexpr auto fields = std::make_tuple(
        fields::Field(&T::color, "color"),
        fields::Field(&T::x, "x"),
        fields::Field(&T::y, "y"));
};


template<template<typename> typename T>
struct PixelTemplate
{
    T<ColorGroup> color;
    T<int> x;
    T<int> y;

    static constexpr auto fields = PixelFields<PixelTemplate>::fields;
    static constexpr auto fieldsTypeName = "Pixel";
};


using PixelGroup = pex::Group<PixelFields, PixelTemplate>;
using PixelModel = typename PixelGroup::Model;



template<typename T>
struct FooFields
{
    static constexpr auto fields = std::make_tuple(
        fields::Field(&T::leftPixel, "leftPixel"),
        fields::Field(&T::rightPixel, "rightPixel"),
        fields::Field(&T::bar, "bar"));
};


template<template<typename> typename T>
struct FooTemplate
{
    T<PixelGroup> leftPixel;
    T<PixelGroup> rightPixel;
    T<int> bar;

    static constexpr auto fields = FooFields<FooTemplate>::fields;
    static constexpr auto fieldsTypeName = "Foo";
};


struct FooCustom
{
    template<typename Base>
    struct Plain: public Base
    {
        Plain()
            :
            Base{}
        {
            this->leftPixel.color.red = 4;
            this->leftPixel.color.green = 5;
            this->leftPixel.color.blue = 6;
        }
    };
};


using FooGroup = pex::Group<FooFields, FooTemplate, FooCustom>;
using FooModel = typename FooGroup::Model;


} // end namespace subgroup


TEST_CASE("Subgroup is initialized using default constructor.", "[groups]")
{
    using Model = subgroup::ColorModel;

    Model model{};

    REQUIRE(model.red.Get() == 1);
    REQUIRE(model.green.Get() == 2);
    REQUIRE(model.blue.Get() == 3);
}


TEST_CASE("Subgroup is initialized by intermediate group.", "[groups]")
{
    using Model = subgroup::FooModel;

    Model model{};

    REQUIRE(model.leftPixel.color.red.Get() == 4);
    REQUIRE(model.leftPixel.color.green.Get() == 5);
    REQUIRE(model.leftPixel.color.blue.Get() == 6);

    REQUIRE(model.rightPixel.color.red.Get() == 1);
    REQUIRE(model.rightPixel.color.green.Get() == 2);
    REQUIRE(model.rightPixel.color.blue.Get() == 3);
}
