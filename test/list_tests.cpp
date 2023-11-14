#include <catch2/catch.hpp>
#include <pex/list.h>
#include <pex/group.h>
#include <nlohmann/json.hpp>


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
            {{1.0, 2.0, 3.0, 5.0}}};
    }
};


TEST_CASE("List as group member", "[List]")
{
    STATIC_REQUIRE(pex::IsMakeList<pex::MakeList<double, 4>>);

    using GrootGroup = pex::Group<GrootFields, GrootTemplate, Groot>;

    using Model = typename GrootGroup::Model;
    using Control = typename GrootGroup::Control;

    Model model;
    Control control(model);
    Control another(control);

    std::cout << fields::DescribeColorized(model.Get()) << std::endl;
    std::cout << fields::DescribeColorized(control.Get()) << std::endl;

    control.values[3].Set(4.0);

    std::cout << fields::DescribeColorized(model.Get()) << std::endl;

    REQUIRE(model.values[3].Get() == 4.0);

    control.values.count.Set(5);
    control.values[4].Set(42.0);
    control.values[2].Set(99.0);

    std::cout << fields::DescribeColorized(another.Get()) << std::endl;

    REQUIRE(model.values[4].Get() == 42.0);
    REQUIRE(another.values[4].Get() == 42.0);

    model.values.count.Set(3);

    REQUIRE(model.values.count.Get() == 3);

    std::cout << fields::DescribeColorized(control.Get()) << std::endl;

    another.values.count.Set(12);

    std::cout << fields::DescribeColorized(control.Get()) << std::endl;
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
using MakeRocketGroup = pex::MakeGroup<RocketGroup>;

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
    T<pex::MakeList<MakeRocketGroup, 4>> rockets;

    static constexpr auto fields = DraxFields<DraxTemplate>::fields;
    static constexpr auto fieldsTypeName = "Drax";
};


using DraxGroup = pex::Group<DraxFields, DraxTemplate>;
using Drax = typename DraxGroup::Plain;

DECLARE_EQUALITY_OPERATORS(Drax);


TEST_CASE("List of groups", "[List]")
{
    STATIC_REQUIRE(pex::IsMakeGroup<MakeRocketGroup>);

    using Model = typename DraxGroup::Model;
    using Control = typename DraxGroup::Control;

    Model model;
    model.name.Set("I am Drax");
    Control control(model);
    Control another(model);

    std::cout << fields::DescribeColorized(model.Get()) << std::endl;
    std::cout << fields::DescribeColorized(control.Get(), 1) << std::endl;

    control.rockets.count.Set(10);
    control.rockets[5].y.Set(31);

    std::cout << '\n' << fields::DescribeColorized(another.Get(), 1) << std::endl;

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

    std::cout << "\nunstructured:\n" << std::setw(4) << unstructured
        << std::endl;

    auto asString = unstructured.dump();

    std::cout << "asString:\n" << asString << std::endl;

    auto recoveredUnstructured = json::parse(asString);
    auto recovered = fields::Structure<Drax>(recoveredUnstructured);

    std::cout << "recovered\n" << fields::DescribeColorized(recovered, 1) << std::endl;

    REQUIRE(recovered == model.Get());
}
