#include <catch2/catch.hpp>
#include <pex/list.h>
#include <pex/group.h>
#include <pex/endpoint.h>
#include <nlohmann/json.hpp>
#include <jive/testing/generator_limits.h>


using json = nlohmann::json;


TEST_CASE("List can change size", "[List]")
{
    using Model = pex::model::Value<int>;
    using Control = pex::control::Value<Model>;

    using ListModel = pex::model::List<Model, 4>;
    using ListControl = pex::control::List<ListModel, Control>;

    ListModel listModel;
    ListControl listControl(listModel);

    REQUIRE(listModel.count.Get() == 4);
    REQUIRE(listControl.count.Get() == 4);
    REQUIRE(listModel.Get().size() == 4);
    REQUIRE(listControl.Get().size() == 4);

    listModel.count.Set(3);

    REQUIRE(listModel.count.Get() == 3);
    REQUIRE(listControl.count.Get() == 3);
    REQUIRE(listModel.Get().size() == 3);
    REQUIRE(listControl.Get().size() == 3);

    listModel.count.Set(12);

    REQUIRE(listModel.count.Get() == 12);
    REQUIRE(listControl.count.Get() == 12);
    REQUIRE(listModel.Get().size() == 12);
    REQUIRE(listControl.Get().size() == 12);
}


template<typename T>
struct GrootFields
{
    static constexpr auto fields = std::make_tuple(
        fields::Field(&T::name, "name"),
        fields::Field(&T::values, "values"));
};

template<template<typename> typename T>
struct GrootTemplate
{
    T<std::string> name;
    T<pex::MakeList<double, 4>> values;

    static constexpr auto fields = GrootFields<GrootTemplate>::fields;
    static constexpr auto fieldsTypeName = "Groot";
};

struct Groot: public GrootTemplate<pex::Identity>
{
    static Groot Default()
    {
        return {
            "I am Groot",
            {{1.0, 2.0, 3.0, 4.0}}};
    }
};

DECLARE_EQUALITY_OPERATORS(Groot)

using GrootGroup = pex::Group<GrootFields, GrootTemplate, Groot>;


TEST_CASE("List as group member", "[List]")
{
    STATIC_REQUIRE(pex::IsMakeList<pex::MakeList<double, 4>>);

    using Model = typename GrootGroup::Model;
    using Control = typename GrootGroup::Control;

    Model model;
    Control control(model);
    Control another(control);

    control.values[3].Set(4.0);

    REQUIRE(model.values[3].Get() == 4.0);

    control.values.count.Set(5);
    control.values[4].Set(42.0);
    control.values[2].Set(99.0);

    REQUIRE(model.values[4].Get() == 42.0);
    REQUIRE(another.values[4].Get() == 42.0);

    model.values.count.Set(3);

    REQUIRE(model.values.count.Get() == 3);

    another.values.count.Set(12);
}


template<typename T>
struct RocketFields
{
    static constexpr auto fields = std::make_tuple(
        fields::Field(&T::x, "x"),
        fields::Field(&T::y, "y"),
        fields::Field(&T::z, "z"));
};

template<template<typename> typename T>
struct RocketTemplate
{
    T<double> x;
    T<double> y;
    T<double> z;

    static constexpr auto fields = RocketFields<RocketTemplate>::fields;
    static constexpr auto fieldsTypeName = "Rocket";
};


using RocketGroup = pex::Group<RocketFields, RocketTemplate>;
using Rocket = typename RocketGroup::Plain;

DECLARE_OUTPUT_STREAM_OPERATOR(Rocket);
DECLARE_EQUALITY_OPERATORS(Rocket);


template<typename T>
struct DraxFields
{
    static constexpr auto fields = std::make_tuple(
        fields::Field(&T::name, "name"),
        fields::Field(&T::rockets, "rockets"));
};

template<template<typename> typename T>
struct DraxTemplate
{
    T<std::string> name;
    T<pex::MakeList<RocketGroup, 4>> rockets;

    static constexpr auto fields = DraxFields<DraxTemplate>::fields;
    static constexpr auto fieldsTypeName = "Drax";
};


using DraxGroup = pex::Group<DraxFields, DraxTemplate>;
using Drax = typename DraxGroup::Plain;

DECLARE_EQUALITY_OPERATORS(Drax);


TEST_CASE("List of groups", "[List]")
{
    STATIC_REQUIRE(pex::IsGroup<RocketGroup>);

    using Model = typename DraxGroup::Model;
    using Control = typename DraxGroup::Control;

    Model model;
    model.name.Set("I am Drax");
    Control control(model);
    Control another(model);

    control.rockets.count.Set(10);
    control.rockets[5].y.Set(31);

    auto drax = another.Get();

    REQUIRE(drax.rockets.size() == 10);
    REQUIRE(drax.rockets[5].y == 31);
}


TEST_CASE("List of groups can be unstructured", "[List]")
{
    using Model = typename DraxGroup::Model;
    using Control = typename DraxGroup::Control;

    Model model;
    model.name.Set("I am Drax");
    Control control(model);

    for (size_t i = 1; i < 5; ++i)
    {
        control.rockets[i - 1].x.Set(static_cast<double>(i * i));
        control.rockets[i - 1].y.Set(static_cast<double>(i * i * i));
        control.rockets[i - 1].z.Set(static_cast<double>(i * i * i * i));
    }

    REQUIRE(model.rockets[2].y.Get() == 27.0);

    auto unstructured = fields::Unstructure<json>(model.Get());
    auto asString = unstructured.dump();
    auto recoveredUnstructured = json::parse(asString);
    auto recovered = fields::Structure<Drax>(recoveredUnstructured);

    REQUIRE(recovered == model.Get());
}


class RocketObserver
{
public:
    using RocketListControl = decltype(DraxGroup::Control::rockets);
    using RocketsEndpoint = pex::Endpoint<RocketObserver, RocketListControl>;
    using RocketList = typename RocketListControl::Type;

    RocketObserver(RocketListControl rocketListControl)
        :
        endpoint_(this, rocketListControl, &RocketObserver::OnRockets_),
        rocketList_(rocketListControl.Get()),
        notificationCount_()
    {

    }

    void OnRockets_(const RocketList &rockets)
    {
        this->rocketList_ = rockets;
        ++this->notificationCount_;
    }

    size_t GetNotificationCount() const
    {
        return this->notificationCount_;
    }

    const RocketList & GetRockets() const
    {
        return this->rocketList_;
    }

    bool operator==(const RocketList &rocketList) const
    {
        if (rocketList.size() != this->rocketList_.size())
        {
            return false;
        }

        for (size_t i = 0; i < rocketList.size(); ++i)
        {
            if (rocketList[i] != this->rocketList_[i])
            {
                return false;
            }
        }

        return true;
    }

private:
    RocketsEndpoint endpoint_;
    RocketList rocketList_;
    size_t notificationCount_;
};


TEST_CASE("List of groups can be observed", "[List]")
{
    using Model = typename DraxGroup::Model;
    using Control = typename DraxGroup::Control;

    Model model;
    model.name.Set("I am Drax");
    Control control(model);
    RocketObserver observer(control.rockets);

    auto values = GENERATE(
        take(
            3,
            chunk(
                15,
                random(-1000.0, 1000.0))));

    std::vector<Rocket> rockets(4);

    for (size_t i = 0; i < 4; ++i)
    {
        rockets.at(i).x = values.at(0 + i * 3);
        rockets.at(i).y = values.at(1 + i * 3);
        rockets.at(i).z = values.at(2 + i * 3);
    }

    control.rockets.Set(rockets);

    REQUIRE(observer == rockets);
    REQUIRE(observer.GetNotificationCount() == 1);

    // Add another rocket.
    rockets.push_back({values.at(12), values.at(13), values.at(14)});

    control.rockets.Set(rockets);

    REQUIRE(observer == rockets);
    REQUIRE(observer.GetNotificationCount() == 2);

    control.rockets.count.Set(3);
    REQUIRE(observer.GetRockets().size() == 3);
}


// A structure that has a list of groups that also contain lists.
template<typename T>
struct GamoraFields
{
    static constexpr auto fields = std::make_tuple(
        fields::Field(&T::name, "name"),
        fields::Field(&T::draxes, "draxes"),
        fields::Field(&T::groots, "groots"));
};


template<template<typename> typename T>
struct GamoraTemplate
{
    T<std::string> name;
    T<pex::MakeList<DraxGroup, 1>> draxes;
    T<pex::MakeList<GrootGroup, 1>> groots;

    static constexpr auto fields = GamoraFields<GamoraTemplate>::fields;
    static constexpr auto fieldsTypeName = "Gamora";
};

using GamoraGroup = pex::Group<GamoraFields, GamoraTemplate>;
using Gamora = typename GamoraGroup::Plain;

DECLARE_EQUALITY_OPERATORS(Gamora);


class GamoraObserver
{
public:
    using GamoraControl = typename GamoraGroup::Control;
    using GamoraEndpoint = pex::Endpoint<GamoraObserver, GamoraControl>;
    using Gamora = typename GamoraControl::Type;

    GamoraObserver(GamoraControl gamoraControl)
        :
        endpoint_(this, gamoraControl, &GamoraObserver::OnGamora_),
        gamora_(gamoraControl.Get()),
        notificationCount_()
    {

    }

    void OnGamora_(const Gamora &gamora)
    {
        this->gamora_ = gamora;
        ++this->notificationCount_;
    }

    size_t GetNotificationCount() const
    {
        return this->notificationCount_;
    }

    const Gamora & GetGamora() const
    {
        return this->gamora_;
    }

private:
    GamoraEndpoint endpoint_;
    Gamora gamora_;
    size_t notificationCount_;
};


TEST_CASE("List of groups member lists can be observed", "[List]")
{
    using Model = typename GamoraGroup::Model;
    using Control = typename GamoraGroup::Control;

    Model model;
    model.name.Set("I am Gamora");
    Control control(model);
    GamoraObserver observer(control);

    auto values = GENERATE(
        take(
            3,
            chunk(
                15,
                random(-1000.0, 1000.0))));

    std::vector<Rocket> rockets(4);

    for (size_t i = 0; i < 4; ++i)
    {
        rockets.at(i).x = values.at(0 + i * 3);
        rockets.at(i).y = values.at(1 + i * 3);
        rockets.at(i).z = values.at(2 + i * 3);
    }

    control.draxes.at(0).rockets.Set(rockets);

    REQUIRE(observer.GetGamora().draxes.at(0).rockets == rockets);
    REQUIRE(observer.GetNotificationCount() == 1);

    // Add another rocket.
    rockets.push_back(
        {values.at(12), values.at(13), values.at(14)});

    control.draxes.at(0).rockets.Set(rockets);

    REQUIRE(observer.GetGamora().draxes.at(0).rockets == rockets);
    REQUIRE(observer.GetNotificationCount() == 2);

    // Change the number of draxes without affecting existing values.
    control.draxes.count.Set(2);
    REQUIRE(observer.GetGamora().draxes.at(0).rockets == rockets);

    control.draxes.at(0).name.Set("I am Drax");

    // std::cout << fields::DescribeColorized(observer.GetGamora(), 1)
    //     << std::endl;
}
