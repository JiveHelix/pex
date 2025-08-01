#include <catch2/catch.hpp>
#include <nlohmann/json.hpp>
#include <jive/vector.h>
#include <pex/list.h>
#include <pex/group.h>
#include <pex/endpoint.h>


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
    static constexpr auto fieldsTypeName = "Test";
};


using TestGroup = pex::Group<TestFields, TestTemplate>;
using TestModel = typename TestGroup::Model;
using TestControl = typename TestGroup::Control;

using ListControl = decltype(TestControl::values);


class TestObserver
{
public:
    using MemberWillRemoveEndpoint =
        pex::Endpoint<TestObserver, typename ListControl::MemberWillRemove>;

    using MemberAddedEndpoint =
        pex::Endpoint<TestObserver, typename ListControl::MemberAdded>;

    TestObserver(TestControl testControl)
        :
        listControl(testControl.values),

        memberWillRemoveEndpoint_(
            this,
            testControl.values.memberWillRemove,
            &TestObserver::OnMemberWillRemove_),

        memberAddedEndpoint_(
            this,
            testControl.values.memberAdded,
            &TestObserver::OnMemberAdded_),

        endpoints(),
        observedValuesByIndex()
    {
        size_t count = testControl.values.count.Get();

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

    void OnValue_(int value, size_t index)
    {
        this->observedValuesByIndex[index] = value;
    }

    void OnMemberWillRemove_(const std::optional<size_t> &index)
    {
        if (!index)
        {
            return;
        }

        jive::SafeErase(this->endpoints, *index);
    }

    void OnMemberAdded_(const std::optional<size_t> &index)
    {
        if (!index)
        {
            return;
        }

        this->endpoints.emplace(
            jive::SafeInsertIterator(this->endpoints, *index),
            this,
            this->listControl[*index],
            &TestObserver::OnValue_,
            *index);
    }

    using BoundEndpoint =
        pex::BoundEndpoint
        <
            typename ListControl::ListItem,
            decltype(&TestObserver::OnValue_)
        >;

    ListControl listControl;
    MemberWillRemoveEndpoint memberWillRemoveEndpoint_;
    MemberAddedEndpoint memberAddedEndpoint_;
    std::vector<BoundEndpoint> endpoints;
    std::map<size_t, int> observedValuesByIndex;
};


} // end namespace endpoint_tests



TEST_CASE("Observe list with BoundEndpoint", "[endpoint]")
{
    using namespace endpoint_tests;
    TestModel model;
    PEX_ROOT(model);

    TestObserver observer{TestControl{model}};

    model.values.count.Set(10);

    REQUIRE(observer.endpoints.size() == 10);

    model.values[5].Set(42);

    REQUIRE(observer.observedValuesByIndex.at(5) == 42);

    model.values.count.Set(32);

    model.values[17].Set(93);

    REQUIRE(observer.observedValuesByIndex.at(17) == 93);
}
