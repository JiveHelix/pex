#include <catch2/catch.hpp>
#include <nlohmann/json.hpp>
#include <pex/list.h>
#include <pex/group.h>
#include <pex/list_observer.h>


using json = nlohmann::json;

namespace endpoint_tests
{

template<typename T>
struct TestFields
{
    static constexpr auto fields = std::make_tuple(
        fields::Field(&T::values, "values"));
};


template<template<typename> typename T>
struct TestTemplate
{
    T<pex::List<int, 0>> values;

    static constexpr auto fields = TestFields<TestTemplate>::fields;
};


using TestGroup = pex::Group<TestFields, TestTemplate>;
using TestModel = typename TestGroup::Model;
using TestControl = typename TestGroup::Control;

using ListControl = decltype(TestControl::values);


class TestObserver
{
public:
    using ListObserver = pex::ListObserver<TestObserver, ListControl>;

    TestObserver(TestControl testControl)
        :
        listControl(testControl.values),
        listObserver(
            this,
            testControl.values,
            &TestObserver::OnCountWillChange_,
            &TestObserver::OnCount_),
        endpoints(),
        observedValuesByIndex()
    {
        this->OnCount_(testControl.values.count.Get());
    }

    void OnValue_(int value, size_t index)
    {
        this->observedValuesByIndex[index] = value;
    }

    void OnCountWillChange_()
    {
        this->endpoints.clear();
    }

    void OnCount_(size_t count)
    {
        this->endpoints.reserve(count);

        for (size_t i = 0; i < count; ++i)
        {
            this->endpoints.emplace_back(
                this,
                this->listControl[i],
                &TestObserver::OnValue_,
                i);
        }
    }

    using BoundEndpoint =
        pex::BoundEndpoint
        <
            typename ListObserver::ListItem,
            decltype(&TestObserver::OnValue_)
        >;

    ListControl listControl;
    ListObserver listObserver;
    std::vector<BoundEndpoint> endpoints;
    std::map<size_t, int> observedValuesByIndex;
};


} // end namespace endpoint_tests



TEST_CASE("Observe list with BoundEndpoint", "[endpoint]")
{
    using namespace endpoint_tests;
    TestModel model;

    TestObserver observer{TestControl{model}};

    model.values.count.Set(10);

    REQUIRE(observer.endpoints.size() == 10);

    model.values[5].Set(42);

    REQUIRE(observer.observedValuesByIndex.at(5) == 42);

    model.values.count.Set(32);

    model.values[17].Set(93);

    REQUIRE(observer.observedValuesByIndex.at(17) == 93);
}
