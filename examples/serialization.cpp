#include <iostream>
#include <fields/fields.h>
#include <pex/pex.h>
#include <pex/endpoint.h>
#include <nlohmann/json.hpp>

using Json = nlohmann::json;

template<typename T>
struct FooFields
{
    static constexpr auto fields = std::make_tuple(
        fields::Field(&T::x, "x"),
        fields::Field(&T::x, "y"),
        fields::Field(&T::x, "z"),
        fields::Field(&T::sayHello, "sayHello"));
};


template<template<typename> typename T>
struct FooTemplate
{
    T<double> x;
    T<double> y;
    T<double> z;
    T<pex::MakeSignal> sayHello;

    static constexpr auto fields = FooFields<FooTemplate>::fields;
    static constexpr auto fieldsTypeName = "Foo";
};


using FooGroup = pex::Group<FooFields, FooTemplate>;
using Foo = typename FooGroup::Plain;
using Model = typename FooGroup::Model;

using Control = typename FooGroup::Control;


class Greeter
{
public:
    static constexpr auto observerName = "Greeter";

    Greeter(const Control &control)
        :
        control_(control),
        sayHello_(this, control.sayHello, &Greeter::SayHello_)
    {

    }

private:
    void SayHello_()
    {
        std::cout << "Hello, world. My x is "
            << this->control_.x.Get() << "!" << std::endl;
    }

    Control control_;
    pex::Endpoint<Greeter, pex::control::Signal<>> sayHello_;
};


int main()
{
    Model model{};
    Control control(model);
    [[maybe_unused]] Greeter greeter(control);

    model.x = 42;
    model.y = 43;
    model.z = 44;

    model.sayHello.Trigger();

    auto asJson = fields::Unstructure<Json>(control.Get());
    auto asString = asJson.dump(4);

    std::cout <<
        "Signals do not appear in JSON data, "
        "because Signals do not manage any data." << std::endl;

    std::cout << asString << std::endl;

    auto recoveredJson = Json::parse(asString);
    auto recovered = fields::Structure<Foo>(recoveredJson);

    std::cout << "Original: " << fields::DescribeCompact(model.Get())
        << std::endl;

    std::cout << "Recovered: " << fields::DescribeCompact(recovered)
        << std::endl;

    return 0;
}
