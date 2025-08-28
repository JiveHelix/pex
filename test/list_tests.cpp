#include <catch2/catch.hpp>
#include <pex/list.h>

#include <pex/ordered_list.h>

#include <pex/group.h>
#include <pex/endpoint.h>
#include <nlohmann/json.hpp>
#include <jive/testing/generator_limits.h>


using json = nlohmann::json;


template<typename List>
class ListChangedObserver
{
public:
    ListChangedObserver(List &list)
        :
        list_(list),

        memberAddedEndpoint_(
            PEX_THIS("ListChangedObserver"),
            list.memberAdded,
            &ListChangedObserver::OnMemberAdded_),

        memberRemovedEndpoint_(
            this,
            list.memberRemoved,
            &ListChangedObserver::OnMemberRemoved_)
    {

    }

    void OnMemberAdded_(const std::optional<size_t> &index)
    {
        if (this->list_.count.Get() != this->list_.size())
        {
            throw std::logic_error(
                "Expected count and list size to be consistent");
        }

        if (index && !(this->list_.size() > *index))
        {
            throw std::logic_error("Expected index to fit within list bounds");
        }
    }

    void OnMemberRemoved_(const std::optional<size_t> &index)
    {
        if (this->list_.count.Get() != this->list_.size())
        {
            throw std::logic_error(
                "Expected count and list size to be consistent");
        }

        if (index && !(this->list_.size() >= *index))
        {
            // The removed index may have been at the end of the list, making
            // the removed index equal to the current list_.size().
            //
            // It must never be beyond this index.
            throw std::logic_error(
                "Expected index to fit within list bounds  ; 1");
        }
    }

private:
    List &list_;

    using MemberAddedControl = typename List::MemberAdded;

    using MemberAddedEndpoint =
        pex::Endpoint<ListChangedObserver, MemberAddedControl>;

    MemberAddedEndpoint memberAddedEndpoint_;

    using MemberRemovedControl = typename List::MemberRemoved;

    using MemberRemovedEndpoint =
        pex::Endpoint<ListChangedObserver, MemberRemovedControl>;

    MemberRemovedEndpoint memberRemovedEndpoint_;
};


TEST_CASE("List can change size", "[List]")
{
    using List = pex::List<int, 4>;
    using ListModel = typename List::Model;
    using ListControl = typename List::template Control<ListModel>;

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


TEST_CASE("List changes size when set.", "[List]")
{
    using List = pex::List<int, 4>;
    using ListModel = typename List::Model;
    using ListControl = typename List::template Control<ListModel>;

    ListModel listModel;
    PEX_ROOT(listModel);

    ListControl listControl(listModel);

    ListChangedObserver observer(listControl);

    REQUIRE(listModel.count.Get() == 4);
    REQUIRE(listControl.count.Get() == 4);
    REQUIRE(listModel.Get().size() == 4);
    REQUIRE(listControl.Get().size() == 4);

    std::vector<int> newValues;
    newValues.resize(8);

    std::iota(
        std::begin(newValues),
        std::end(newValues),
        0);

    listModel.Set(newValues);

    REQUIRE(listModel.count.Get() == 8);
    REQUIRE(listControl.count.Get() == 8);
    REQUIRE(listModel.Get().size() == 8);
    REQUIRE(listControl.Get().size() == 8);

    newValues.resize(6);

    listModel.Set(newValues);

    REQUIRE(listModel.count.Get() == 6);
    REQUIRE(listControl.count.Get() == 6);
    REQUIRE(listModel.Get().size() == 6);
    REQUIRE(listControl.Get().size() == 6);
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
    using Control = typename GrootGroup::template Control<Model>;

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
using RocketModel = typename RocketGroup::Model;
using RocketControl = typename RocketGroup::template Control<RocketModel>;

DECLARE_OUTPUT_STREAM_OPERATOR(Rocket)
DECLARE_EQUALITY_OPERATORS(Rocket)


template<typename T>
struct DraxFields
{
    static constexpr auto fields = std::make_tuple(
        fields::Field(&T::name, "name"),
        fields::Field(&T::rockets, "rockets"));
};


struct ListTag {};
struct OrderedListTag {};


template<typename Tag>
struct ChooseRocketList_;

template<>
struct ChooseRocketList_<ListTag>
{
    using type = pex::List<RocketGroup, 4>;
};

template<>
struct ChooseRocketList_<OrderedListTag>
{
    using type = pex::OrderedListGroup<pex::List<RocketGroup, 4>>;
};

template<typename T>
using ChooseRocketList = typename ChooseRocketList_<T>::type;


template<typename Tag>
struct DraxTemplate
{
    template<template<typename> typename T>
    struct Template
    {
        T<std::string> name;
        T<ChooseRocketList<Tag>> rockets;

        static constexpr auto fields = DraxFields<Template>::fields;
        static constexpr auto fieldsTypeName = "Drax";
    };
};

template<typename Tag>
using DraxGroup =
    pex::Group<DraxFields, DraxTemplate<Tag>::template Template>;


template<typename Tag>
using Drax = typename DraxGroup<Tag>::Plain;


DECLARE_EQUALITY_OPERATORS(Drax<ListTag>)
DECLARE_EQUALITY_OPERATORS(Drax<OrderedListTag>)


TEMPLATE_TEST_CASE("List of groups", "[List]", ListTag, OrderedListTag)
{
    STATIC_REQUIRE(pex::IsGroup<RocketGroup>);

    using GroupUnderTest = DraxGroup<TestType>;
    using Model = typename GroupUnderTest::Model;
    using Control = typename GroupUnderTest::template Control<Model>;

    Model model;

    if constexpr (std::is_same_v<TestType, OrderedListTag>)
    {
        REQUIRE(model.rockets.list.count.Get() == 4);
        REQUIRE(model.rockets.indices.Get().size() == 4);
    }

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


TEMPLATE_TEST_CASE(
    "List of groups can be unstructured",
    "[List]",
    ListTag,
    OrderedListTag)
{
    using GroupUnderTest = DraxGroup<TestType>;
    using Model = typename GroupUnderTest::Model;
    using Control = typename GroupUnderTest::template Control<Model>;

    Model model;
    model.name.Set("I am Drax");
    Control control(model);

    if constexpr (std::is_same_v<TestType, OrderedListTag>)
    {
        REQUIRE(model.rockets.list.count.Get() == 4);
        REQUIRE(model.rockets.indices.Get().size() == 4);
    }
    else
    {
        REQUIRE(model.rockets.count.Get() == 4);
    }

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
    auto recovered = fields::Structure<Drax<TestType>>(recoveredUnstructured);

    REQUIRE(recovered == model.Get());
}


template<typename Tag>
class RocketListObserver
{
public:
    using DraxModel = typename DraxGroup<Tag>::Model;
    using DraxControl = typename DraxGroup<Tag>::template Control<DraxModel>;
    using RocketListControl = decltype(DraxControl::rockets);

    using RocketsEndpoint =
        pex::Endpoint<RocketListObserver, RocketListControl>;

    using RocketList = typename RocketListControl::Type;

    RocketListObserver(RocketListControl rocketListControl)
        :
        endpoint_(
            PEX_THIS("RocketListObserver"),
            rocketListControl,
            &RocketListObserver::OnRockets_),

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

private:
    RocketsEndpoint endpoint_;
    RocketList rocketList_;
    size_t notificationCount_;
};



TEMPLATE_TEST_CASE(
    "List of groups can be observed",
    "[List]",
    ListTag,
    OrderedListTag)
{
    using GroupUnderTest = DraxGroup<TestType>;
    using Model = typename GroupUnderTest::Model;
    using Control = typename GroupUnderTest::template Control<Model>;

    Model model;
    model.name.Set("I am Drax");
    Control control(model);
    RocketListObserver<TestType> observer(control.rockets);

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
    REQUIRE(model.rockets.Get().size() == 3);
    REQUIRE(observer.GetNotificationCount() == 3);
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


template<typename Tag>
struct GamoraTemplate
{
    template<template<typename> typename T>
    struct Template
    {
        T<std::string> name;
        T<pex::List<DraxGroup<Tag>, 1>> draxes;
        T<pex::List<GrootGroup, 1>> groots;

        static constexpr auto fields = GamoraFields<Template>::fields;
        static constexpr auto fieldsTypeName = "Gamora";
    };
};


template<typename Tag>
using GamoraGroup =
    pex::Group<GamoraFields, GamoraTemplate<Tag>::template Template>;

template<typename Tag>
using Gamora = typename GamoraGroup<Tag>::Plain;


DECLARE_EQUALITY_OPERATORS(Gamora<ListTag>)
DECLARE_EQUALITY_OPERATORS(Gamora<OrderedListTag>)


template<typename Tag>
class GamoraObserver
{
public:
    using GamoraModel = typename GamoraGroup<Tag>::Model;

    using GamoraControl =
        typename GamoraGroup<Tag>::template Control<GamoraModel>;

    using GamoraEndpoint = pex::Endpoint<GamoraObserver, GamoraControl>;
    using Gamora = typename GamoraControl::Type;

    GamoraObserver(GamoraControl gamoraControl)
        :
        endpoint_(
            PEX_THIS("GamoraObserver"),
            gamoraControl,
            &GamoraObserver::OnGamora_),

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


TEMPLATE_TEST_CASE(
    "List of groups with member lists can be observed",
    "[List]",
    ListTag,
    OrderedListTag)
{
    using GroupUnderTest = GamoraGroup<TestType>;
    using Model = typename GroupUnderTest::Model;
    using Control = typename GroupUnderTest::template Control<Model>;

    Model model;
    model.name.Set("I am Gamora");
    Control control(model);
    GamoraObserver<TestType> observer(control);

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

    if constexpr (std::is_same_v<TestType, OrderedListTag>)
    {
        REQUIRE(observer.GetGamora().draxes.at(0).rockets.list == rockets);
    }
    else
    {
        REQUIRE(observer.GetGamora().draxes.at(0).rockets == rockets);
    }

    REQUIRE(observer.GetNotificationCount() == 1);

    // Add another rocket.
    rockets.push_back(
        {values.at(12), values.at(13), values.at(14)});

    control.draxes.at(0).rockets.Set(rockets);

    if constexpr (std::is_same_v<TestType, OrderedListTag>)
    {
        REQUIRE(observer.GetGamora().draxes.at(0).rockets.list == rockets);
    }
    else
    {
        REQUIRE(observer.GetGamora().draxes.at(0).rockets == rockets);
    }

    REQUIRE(observer.GetNotificationCount() == 2);

    // Change the number of draxes without affecting existing values.
    control.draxes.count.Set(2);

    if constexpr (std::is_same_v<TestType, OrderedListTag>)
    {
        REQUIRE(observer.GetGamora().draxes.at(0).rockets.list == rockets);
    }
    else
    {
        REQUIRE(observer.GetGamora().draxes.at(0).rockets == rockets);
    }

    // Unstructure/Structure to copy of the model.
    Model secondModel;

    auto unstructured = fields::Unstructure<json>(model.Get());
    auto asString = unstructured.dump();
    auto recoveredUnstructured = json::parse(asString);
    auto recovered = fields::Structure<Gamora<TestType>>(recoveredUnstructured);

    secondModel.Set(recovered);

    REQUIRE(secondModel.Get() == model.Get());
}


template<typename Control, typename Tag, typename = void>
struct ChooseListControl_
{
    using type = Control;
};

template<typename Control, typename Tag>
struct ChooseListControl_
<
    Control,
    Tag,
    std::enable_if_t<std::is_same_v<Tag, OrderedListTag>>
>
{
    using type = decltype(Control::list);
};


template<typename Control, typename Tag>
using ChooseListControl = typename ChooseListControl_<Control, Tag>::type;


template<typename Tag, typename Control>
auto & GetList(Control &control)
{
    if constexpr (std::is_same_v<Tag, OrderedListTag>)
    {
        return control.list;
    }
    else
    {
        return control;
    }
}


template<typename Tag>
class RocketSignalObserver
{
public:
    using DraxModel = typename DraxGroup<Tag>::Model;
    using DraxControl = typename DraxGroup<Tag>::template Control<DraxModel>;
    using RocketsControl = decltype(DraxControl::rockets);
    using RocketListControl = ChooseListControl<RocketsControl, Tag>;

    using RocketsConnect =
        pex::detail::ListConnect
        <
            RocketSignalObserver,
            RocketListControl,
            pex::ControlSelector
        >;

    RocketSignalObserver(RocketsControl rocketsControl)
        :
        endpoint_(
            this,
            GetList<Tag>(rocketsControl),
            &RocketSignalObserver::OnRockets_),

        notificationCount_()
    {

    }

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


TEMPLATE_TEST_CASE(
    "List of groups can be Set",
    "[List]",
    ListTag,
    OrderedListTag)
{
    using Model = typename DraxGroup<TestType>::Model;
    using Control = typename DraxGroup<TestType>::template Control<Model>;

    Model model;
    model.name.Set("I am Drax");
    Control control(model);
    RocketListObserver<TestType> observer(control.rockets);
    RocketSignalObserver<TestType> signalObserver(control.rockets);

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
        endpoint_(
            PEX_THIS("RocketObserver"),
            rocketControl,
            &RocketObserver::OnRocket_),

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
    using Control = typename StarLordGroup::template Control<Model>;

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
    using Control = pex::List<int>::template Control<Model>;

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
