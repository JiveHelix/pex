#include <catch2/catch.hpp>
#include <fields/fields.h>
#include <pex/select.h>
#include <pex/group.h>
#include <pex/endpoint.h>




TEST_CASE("Select::Get returns value, not index", "[select]")
{
    using Select = pex::model::Select<double>;

    Select select({1.0, 2.78, 3.14, 42.0});
    REQUIRE(select.Get() == Approx(1.0));

    select.SetSelection(1);
    REQUIRE(select.Get() == Approx(2.78));

    select.SetSelection(3);
    REQUIRE(select.Get() == Approx(42.0));
}


template<typename T>
struct SomeFields
{
    static constexpr auto fields = std::make_tuple(
        fields::Field(&T::x, "x"),
        fields::Field(&T::y, "y"),
        fields::Field(&T::rate, "rate"));
};

template<template<typename> typename T>
struct SomeTemplate
{
    T<double> x;
    T<double> y;
    T<pex::MakeSelect<double>> rate;

    static constexpr auto fields = SomeFields<SomeTemplate>::fields;
};

using SomeGroup = pex::Group<SomeFields, SomeTemplate>;
using SomeModel = typename SomeGroup::Model;
using SomeControl = typename SomeGroup::Control;
using SomePlain = typename SomeGroup::Plain;

static_assert(pex::IsModelSelect<decltype(SomeModel::rate)>);


TEST_CASE("Select is member of Group", "[select]")
{
    SomePlain plain{{1.0, 2.0, 42.0}};
    SomeModel model(plain);

    REQUIRE(SomeControl(model).rate.selection.Get() == 0);
    REQUIRE(model.rate.Get() == Approx(42.0));

    model.rate.SetChoices({1.0, 2.78, 3.14, 42.0});

    // Setting the choices should leave the selection index unchanged...
    REQUIRE(SomeControl(model).rate.selection.Get() == 0);

    // but the value will change unless the choices at the selected index is
    // the same.
    REQUIRE(model.rate.Get() == Approx(1.0));

    model.rate.SetSelection(1);
    REQUIRE(model.rate.Get() == Approx(2.78));

    model.rate.SetSelection(3);
    REQUIRE(model.rate.Get() == Approx(42.0));
}


using SomeControl = typename SomeGroup::Control;

template<typename Observer>
using SomeEndpointGroup = pex::EndpointGroup<Observer, SomeControl>;


struct TestObserver
{
    static constexpr auto observerName = "select_tests::TestObserver";

    double observedRate;
    SomeEndpointGroup<TestObserver> endpoints_;

    TestObserver(SomeControl control)
        :
        observedRate{control.rate.Get()},
        endpoints_{this, control}
    {
        this->endpoints_.rate.Connect(&TestObserver::OnObserve_);
    }

    void OnObserve_(double rate)
    {
        this->observedRate = rate;
    }
};


TEST_CASE("Select observer is notified.", "[select]")
{
    SomeModel model{};
    model.rate.SetChoices({1.0, 2.78, 3.14, 42.0});

    REQUIRE(model.rate.Get() == Approx(1.0));
    REQUIRE(SomeControl(model).rate.value.Get() == Approx(1.0));

    TestObserver observer((SomeControl(model)));

    REQUIRE(observer.observedRate == Approx(1.0));

    model.rate.SetSelection(1);
    REQUIRE(observer.observedRate == Approx(2.78));

    model.rate.SetSelection(3);
    REQUIRE(observer.observedRate == Approx(42.0));
}
