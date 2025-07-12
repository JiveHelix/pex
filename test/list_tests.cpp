#include <catch2/catch.hpp>
#include <pex/list.h>

#include <pex/ordered_list.h>

#include <pex/group.h>
#include <pex/endpoint.h>
#include <nlohmann/json.hpp>
#include <jive/testing/generator_limits.h>


using json = nlohmann::json;


#define TEST_WITH_ORDERED_LIST



TEST_CASE("List can change size", "[List]")
{
    using List = pex::List<int, 4>;
    using ListModel = typename List::Model;
    using ListControl = typename List::Control;

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
    T<pex::List<double, 4>> values;

    static constexpr auto fields = GrootFields<GrootTemplate>::fields;
    static constexpr auto fieldsTypeName = "Groot";
};

struct GroupTypes
{
    template<typename Base>
    struct Plain: public Base
    {
        Plain()
            :
            Base{
            "I am Groot",
            {{1.0, 2.0, 3.0, 4.0}}}
        {

        }
    };
};

using GrootGroup = pex::Group<GrootFields, GrootTemplate, GroupTypes>;
using Groot = typename GrootGroup::Plain;

DECLARE_EQUALITY_OPERATORS(Groot)
DECLARE_EQUALITY_OPERATORS(GrootTemplate<pex::Identity>)


TEST_CASE("List as group member", "[List]")
{
    STATIC_REQUIRE(pex::IsList<pex::List<double, 4>>);

    using Model = typename GrootGroup::Model;
    using Control = typename GrootGroup::Control;

    Model model;

    REQUIRE(model.values.at(2).Get() == 3.0);

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
using RocketControl = typename RocketGroup::Control;

DECLARE_OUTPUT_STREAM_OPERATOR(Rocket)
DECLARE_EQUALITY_OPERATORS(Rocket)


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

#ifdef TEST_WITH_ORDERED_LIST
    T<pex::OrderedListGroup<pex::List<RocketGroup, 4>>> rockets;
#else
    T<pex::List<RocketGroup, 4>> rockets;
#endif

    static constexpr auto fields = DraxFields<DraxTemplate>::fields;
    static constexpr auto fieldsTypeName = "Drax";
};


using DraxGroup = pex::Group<DraxFields, DraxTemplate>;
using Drax = typename DraxGroup::Plain;

DECLARE_EQUALITY_OPERATORS(Drax)


TEST_CASE("List of groups", "[List]")
{
    STATIC_REQUIRE(pex::IsGroup<RocketGroup>);

    using Model = typename DraxGroup::Model;
    using Control = typename DraxGroup::Control;

    Model model;

#ifdef TEST_WITH_ORDERED_LIST
    REQUIRE(model.rockets.list.count.Get() == 4);
    REQUIRE(model.rockets.indices.Get().size() == 4);
#endif

    REQUIRE(model.rockets.count.Get() == 4);

    model.name.Set("I am Drax");
    Control control(model);
    Control another(model);

    control.rockets.count.Set(10);
    REQUIRE(model.rockets.count.Get() == 10);
    REQUIRE(model.rockets.size() == 10);
    REQUIRE(control.rockets.size() == 10);

    REQUIRE(another.rockets.count.Get() == 10);
    REQUIRE(another.rockets.size() == 10);

    control.rockets[5].y.Set(31);

    REQUIRE(control.rockets[5].y.Get() == 31);
    REQUIRE(model.rockets[5].y.Get() == 31);
    REQUIRE(another.rockets[5].y.Get() == 31);

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


#ifdef TEST_WITH_ORDERED_LIST
    REQUIRE(model.rockets.list.count.Get() == 4);
    REQUIRE(model.rockets.indices.Get().size() == 4);
#else
    REQUIRE(model.rockets.count.Get() == 4);
#endif

    REQUIRE(model.rockets.count.Get() == 4);

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


class RocketListObserver
{
public:
    using RocketListControl = decltype(DraxGroup::Control::rockets);

    using RocketsEndpoint =
        pex::Endpoint<RocketListObserver, RocketListControl>;

    using RocketList = typename RocketListControl::Type;

    RocketListObserver(RocketListControl rocketListControl)
        :
        endpoint_(this, rocketListControl, &RocketListObserver::OnRockets_),
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

#ifdef TEST_WITH_ORDERED_LIST
    bool operator==(const std::vector<Rocket> &rocketList) const
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
#endif

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
    RocketListObserver observer(control.rockets);

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
    REQUIRE(model.rockets.count.Get() == 3);
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
    T<pex::List<DraxGroup, 1>> draxes;
    T<pex::List<GrootGroup, 1>> groots;

    static constexpr auto fields = GamoraFields<GamoraTemplate>::fields;
    static constexpr auto fieldsTypeName = "Gamora";
};

using GamoraGroup = pex::Group<GamoraFields, GamoraTemplate>;
using Gamora = typename GamoraGroup::Plain;

DECLARE_EQUALITY_OPERATORS(Gamora)


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


TEST_CASE("List of groups with member lists can be observed", "[List]")
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

#ifdef TEST_WITH_ORDERED_LIST
    REQUIRE(observer.GetGamora().draxes.at(0).rockets.list == rockets);
#else
    REQUIRE(observer.GetGamora().draxes.at(0).rockets == rockets);
#endif

    REQUIRE(observer.GetNotificationCount() == 1);

    // Add another rocket.
    rockets.push_back(
        {values.at(12), values.at(13), values.at(14)});

    control.draxes.at(0).rockets.Set(rockets);

#ifdef TEST_WITH_ORDERED_LIST
    REQUIRE(observer.GetGamora().draxes.at(0).rockets.list == rockets);
#else
    REQUIRE(observer.GetGamora().draxes.at(0).rockets == rockets);
#endif

    REQUIRE(observer.GetNotificationCount() == 2);

    // Change the number of draxes without affecting existing values.
    control.draxes.count.Set(2);

#ifdef TEST_WITH_ORDERED_LIST
    REQUIRE(observer.GetGamora().draxes.at(0).rockets.list == rockets);
#else
    REQUIRE(observer.GetGamora().draxes.at(0).rockets == rockets);
#endif

    // Unstructure/Structure to copy of the model.
    Model secondModel;

    auto unstructured = fields::Unstructure<json>(model.Get());
    auto asString = unstructured.dump();
    auto recoveredUnstructured = json::parse(asString);
    auto recovered = fields::Structure<Gamora>(recoveredUnstructured);

    secondModel.Set(recovered);

    REQUIRE(secondModel.Get() == model.Get());
}


class RocketSignalObserver
{
public:
#ifdef TEST_WITH_ORDERED_LIST
    using RocketsControl = decltype(DraxGroup::Control::rockets);
    using RocketListControl = decltype(RocketsControl::list);

    using RocketsConnect =
        pex::detail::ListConnect<RocketSignalObserver, RocketListControl>;

    RocketSignalObserver(RocketsControl rocketsControl)
        :
        endpoint_(this, rocketsControl.list, &RocketSignalObserver::OnRockets_),
        notificationCount_()
    {

    }
#else
    using RocketListControl = decltype(DraxGroup::Control::rockets);

    using RocketsConnect =
        pex::detail::ListConnect<RocketSignalObserver, RocketListControl>;

    RocketSignalObserver(RocketListControl rocketListControl)
        :
        endpoint_(this, rocketListControl, &RocketSignalObserver::OnRockets_),
        notificationCount_()
    {

    }
#endif

    void OnRockets_()
    {
        ++this->notificationCount_;
    }

    size_t GetNotificationCount() const
    {
        return this->notificationCount_;
    }

private:
    RocketsConnect endpoint_;
    size_t notificationCount_;
};


#ifndef TEST_WITH_ORDERED_LIST
TEST_CASE("List of groups can be Set", "[List]")
{
    using Model = typename DraxGroup::Model;
    using Control = typename DraxGroup::Control;

    Model model;
    model.name.Set("I am Drax");
    Control control(model);
    RocketListObserver observer(control.rockets);
    RocketSignalObserver signalObserver(control.rockets);

    auto values = GENERATE(
        take(
            1,
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
    REQUIRE(signalObserver.GetNotificationCount() == 1);

    // Add another rocket.
    rockets.push_back({values.at(12), values.at(13), values.at(14)});

    auto drax = control.Get();
    drax.rockets = rockets;

    // We haven't changed the model yet, so the observer should be untouched.
    REQUIRE(observer.GetNotificationCount() == 1);
    REQUIRE(signalObserver.GetNotificationCount() == 1);

    REQUIRE(rockets.size() == 5);

    // This call will update the model, and notify the observer.
    control.Set(drax);

    REQUIRE(observer == rockets);

    // We updated the list all at once, so we expect only one notification.
    REQUIRE(observer.GetNotificationCount() == 2);
    REQUIRE(signalObserver.GetNotificationCount() == 2);
}
#endif


template<typename T>
struct StarLordFields
{
    static constexpr auto fields = std::make_tuple(
        fields::Field(&T::name, "name"),
        fields::Field(&T::rocket, "rocket"));
};

template<template<typename> typename T>
struct StarLordTemplate
{
    T<std::string> name;
    T<RocketGroup> rocket;

    static constexpr auto fields = StarLordFields<StarLordTemplate>::fields;
    static constexpr auto fieldsTypeName = "StarLord";
};


using StarLordGroup = pex::Group<StarLordFields, StarLordTemplate>;
using StarLord = typename StarLordGroup::Plain;

DECLARE_EQUALITY_OPERATORS(StarLord)


class RocketObserver
{
public:
    using RocketsEndpoint =
        pex::Endpoint<RocketObserver, RocketControl>;

    RocketObserver(RocketControl rocketControl)
        :
        endpoint_(this, rocketControl, &RocketObserver::OnRocket_),
        rocket_(rocketControl.Get()),
        notificationCount_()
    {

    }

    void OnRocket_(const Rocket &rocket)
    {
        this->rocket_ = rocket;
        ++this->notificationCount_;
    }

    size_t GetNotificationCount() const
    {
        return this->notificationCount_;
    }

    const Rocket & GetRocket() const
    {
        return this->rocket_;
    }

    bool operator==(const Rocket &rocket) const
    {
        return this->rocket_ == rocket;
    }

private:
    RocketsEndpoint endpoint_;
    Rocket rocket_;
    size_t notificationCount_;
};


TEST_CASE("Subgroup notification happens once.", "[List]")
{
    using Model = typename StarLordGroup::Model;
    using Control = typename StarLordGroup::Control;

    Model model;
    model.name.Set("I am Star-Lord");
    Control control(model);
    RocketObserver observer(control.rocket);

    auto values = GENERATE(
        take(
            1,
            chunk(
                6,
                random(-1000.0, 1000.0))));

    Rocket rocket;

    for (size_t i = 0; i < 3; ++i)
    {
        rocket.x = values.at(0);
        rocket.y = values.at(1);
        rocket.z = values.at(2);
    }

    control.rocket.Set(rocket);

    REQUIRE(observer == rocket);
    REQUIRE(observer.GetNotificationCount() == 1);

    // Change rocket.
    rocket = Rocket{values.at(3), values.at(4), values.at(5)};

    auto starLord = control.Get();
    starLord.rocket = rocket;

    // We haven't changed the model yet, so the observer should be untouched.
    REQUIRE(observer.GetNotificationCount() == 1);

    // This call will update the model, and notify the observer.
    control.Set(starLord);

    REQUIRE(observer == rocket);

    // We updated the list all at once, so we expect only one notification.
    REQUIRE(observer.GetNotificationCount() == 2);
}


TEST_CASE("Delete selected.", "[List]")
{
    using Model = pex::List<int>::Model;
    using Control = pex::List<int>::Control;

    Model model;
    Control control(model);

    model.Set(std::vector<int>({0, 1, 2, 3, 4}));
    model.selected.Set(2);

    REQUIRE(control[*control.selected.Get()].Get() == 2);

    model.EraseSelected();

    REQUIRE(!control.selected.Get());
    REQUIRE(control.count.Get() == 4);
    REQUIRE(control[2].Get() == 3);
}


TEST_CASE("ValueContainer allows operator[] access", "[List]")
{
    using Model = pex::ModelSelector<std::vector<int>>;

    Model model;
    std::vector<int> values(10);
    std::iota(std::begin(values), std::end(values), 42);

    model.Set(values);

    REQUIRE(model[4] == 46);
}


TEST_CASE("KeyValueContainer allows key access", "[List]")
{
    using Model = pex::ModelSelector<std::map<std::string, int>>;

    Model model;
    model.Set("foo", 42);

    REQUIRE(model.at("foo") == 42);
}
