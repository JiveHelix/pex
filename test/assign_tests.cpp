
#include <catch2/catch.hpp>
#include <fields/fields.h>
#include <fields/assign.h>
#include <string>
#include <optional>

#include "pex/pex.h"


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
struct AssignTestFields
{
    static constexpr auto fields = std::make_tuple(
        fields::Field(&T::foo, "foo"),
        fields::Field(&T::wibble, "wibble"),
        fields::Field(&T::wobble, "wobble"));
};


template<template<typename> typename T>
struct AssignTestTemplate
{
    T<uint16_t> foo;
    T<double> wibble;
    T<std::string> wobble;

    static constexpr auto fields = AssignTestFields<AssignTestTemplate>::fields;
};


struct AssignPlain: public AssignTestTemplate<pex::Identity>
{

};


struct AssignTestModel: public AssignTestTemplate<pex::Model>
{
    AssignPlain GetTest()
    {
        AssignPlain result{};
        fields::AssignConvert<AssignTestFields>(result, *this);
        return result;
    }

    void SetTest(const AssignPlain &test)
    {
        fields::Assign<AssignTestFields>(*this, test);
    }
};


struct AssignTestControl:
    public AssignTestTemplate<pex::Control<void>::template Type>
{
public:
    AssignTestControl(AssignTestModel &model)
    {
        fields::AssignConvert<AssignTestFields>(*this, model);
    }

};


DECLARE_OUTPUT_STREAM_OPERATOR(AssignPlain)


TEST_CASE("Assign round trip.", "[pex]")
{
    AssignPlain test{{42, 3.14159, "frob"}};

    AssignTestModel model{};

    model.SetTest(test);

    AssignPlain check = model.GetTest();
    REQUIRE(test == check);
}


TEST_CASE("Assign is observed.", "[pex]")
{
    AssignPlain test{{42, 3.14159, "frob"}};

    AssignTestModel model{};

    auto observer = Observer(AssignTestControl(model).foo);

    model.SetTest(test);
    
    REQUIRE(!!observer.observedValue);
    REQUIRE(observer.observedValue == test.foo);
}


TEST_CASE("Assign to control reaches model.", "[pex]")
{
    AssignPlain test{{42, 3.14159, "frob"}};

    AssignTestModel model{};

    AssignTestControl control(model);

    fields::Assign<AssignTestFields>(control, test);

    AssignPlain check = model.GetTest();
    REQUIRE(test == check);
}

