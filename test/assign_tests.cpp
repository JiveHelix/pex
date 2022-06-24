
#include <catch2/catch.hpp>
#include <fields/fields.h>
#include <fields/assign.h>
#include <string>
#include <optional>

#include "pex/interface.h"


#include <fields/comparisons.h>


template<typename Control>
class Observer
{
public:
    using Type = typename Control::Type;

    Observer(Control control)
        :
        control_(this, control)
    {
        PEX_LOG("Connect");
        this->control_.Connect(&Observer::Observe_);
    }

    void Set(pex::Argument<Type> value)
    {
        this->control_.Set(value);
    }

private:
    void Observe_(pex::Argument<Type> value)
    {
        this->observedValue = value;
    }

    pex::Terminus<Observer, Control> control_;

public:
    std::optional<Type> observedValue;
};


template<typename T>
struct TestFields
{
    static constexpr auto fields = std::make_tuple(
        fields::Field(&T::foo, "foo"),
        fields::Field(&T::wibble, "wibble"),
        fields::Field(&T::wobble, "wobble"));
};


template<template<typename> typename T>
struct TestTemplate
{
    T<uint16_t> foo;
    T<double> wibble;
    T<std::string> wobble;

    static constexpr auto fields = TestFields<TestTemplate>::fields;
};


struct Test: public TestTemplate<pex::Identity>
{

};


struct TestModel: public TestTemplate<pex::Model>
{
    Test GetTest()
    {
        Test result{};
        fields::AssignConvert<TestFields>(result, *this);
        return result;
    }

    void SetTest(const Test &test)
    {
        fields::Assign<TestFields>(*this, test);
    }
};


struct TestControl:
    public TestTemplate<pex::Control<void>::template Type>
{
public:
    TestControl(TestModel &model)
    {
        fields::AssignConvert<TestFields>(*this, model);
    }

};


DECLARE_OUTPUT_STREAM_OPERATOR(Test)


TEST_CASE("Assign round trip.", "[pex]")
{
    Test test{{42, 3.14159, "frob"}};

    TestModel model{};

    model.SetTest(test);

    Test check = model.GetTest();
    REQUIRE(test == check);
}


TEST_CASE("Assign is observed.", "[pex]")
{
    Test test{{42, 3.14159, "frob"}};

    TestModel model{};

    auto observer = Observer(TestControl(model).foo);

    model.SetTest(test);
    
    REQUIRE(!!observer.observedValue);
    REQUIRE(observer.observedValue == test.foo);
}


TEST_CASE("Assign to control reaches model.", "[pex]")
{
    Test test{{42, 3.14159, "frob"}};

    TestModel model{};

    TestControl control(model);

    fields::Assign<TestFields>(control, test);

    Test check = model.GetTest();
    REQUIRE(test == check);
}

