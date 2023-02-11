#include <iostream>
#include <fields/fields.h>
#include <pex/pex.h>
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

template<typename Observer>
using Terminus = typename FooGroup::Terminus<Observer>;


class Greeter
{
public:
    Greeter(Terminus<void> &terminus)
        :
        terminus_(this, terminus)
    {
        this->terminus_.sayHello.Connect(&Greeter::SayHello_);
    }

private:
    void SayHello_()
    {
        std::cout << "Hello, world. My x is "
            << this->terminus_.x.Get() << "!" << std::endl;
    }

    Terminus<Greeter> terminus_;
};

int main()
{
    Model model{};
    Terminus<void> terminus(nullptr, model);
    [[maybe_unused]] Greeter greeter(terminus);

    model.x = 42;
    model.y = 43;
    model.z = 44;

    model.sayHello.Trigger();

    auto asJson = fields::Unstructure<Json>(terminus.Get());
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
