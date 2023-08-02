#include <iostream>
#include <pex/group.h>
#include <fields/fields.h>
#include <pex/endpoint.h>


template<typename T>
struct WeaponsFields
{
    static constexpr auto fields = std::make_tuple(
        fields::Field(&T::firstFruit, "firstFruit"),
        fields::Field(&T::secondFruit, "secondFruit"),
        fields::Field(&T::notFruit, "notFruit"));
};


template<template<typename> typename T>
struct WeaponsTemplate
{
    T<std::string> firstFruit;
    T<std::string> secondFruit;
    T<std::string> notFruit;

    static constexpr auto fields = WeaponsFields<WeaponsTemplate>::fields;
    static constexpr auto fieldsTypeName = "Weapons";
};


using WeaponsGroup = pex::Group<WeaponsFields, WeaponsTemplate>;
using WeaponsPlain = typename WeaponsGroup::Plain;
using WeaponsModel = typename WeaponsGroup::Model;

using WeaponsControl = typename WeaponsGroup::Control;


template<typename T>
struct GpsFields
{
    static constexpr auto fields = std::make_tuple(
        fields::Field(&T::time, "time"),
        fields::Field(&T::latitude, "latitude"),
        fields::Field(&T::longitude, "longitude"),
        fields::Field(&T::elevation, "elevation"));
};


template<template<typename> typename T>
struct GpsTemplate
{
    T<int64_t> time;
    T<double> latitude;
    T<double> longitude;
    T<double> elevation;

    static constexpr auto fields = GpsFields<GpsTemplate>::fields;
    static constexpr auto fieldsTypeName = "Gps";
};


using GpsGroup = pex::Group<GpsFields, GpsTemplate>;
using GpsPlain = typename GpsGroup::Plain;
using GpsModel = typename GpsGroup::Model;

using GpsControl = typename GpsGroup::Control;


inline GpsPlain DefaultGps()
{
    return {{
        1334706453,
        40.56923581063791,
        -111.63928609736942,
        3322.0}};
}


// A struct that contains groups.
template<typename T>
struct AggregateFields
{
    static constexpr auto fields = std::make_tuple(
        fields::Field(&T::airspeedVelocity, "airspeedVelocity"),
        fields::Field(&T::weapons, "weapons"),
        fields::Field(&T::gps, "gps"));
};


template<template<typename> typename T>
struct AggregateTemplate
{
    T<double> airspeedVelocity;
    T<pex::MakeGroup<WeaponsGroup>> weapons;
    T<pex::MakeGroup<GpsGroup>> gps;

    static constexpr auto fields = AggregateFields<AggregateTemplate>::fields;
    static constexpr auto fieldsTypeName = "Aggregate";
};


using AggregateGroup = pex::Group<AggregateFields, AggregateTemplate>;
using AggregateModel = typename AggregateGroup::Model;
using AggregateControl = typename AggregateGroup::Control;


class WeaponsObserver
{
public:
    WeaponsObserver(WeaponsControl control)
        :
        endpoint_(this, control, &WeaponsObserver::OnWeapons)
    {

    }

    void OnWeapons(const WeaponsPlain &weapons)
    {
        std::cout << "OnWeapons: " << fields::DescribeColorized(weapons, 1)
              << std::endl;
    }

private:
    pex::Endpoint<WeaponsObserver, WeaponsControl> endpoint_;
};


int main()
{
    AggregateModel model;

    std::cout << "model.weapons.firstFruit: "
        << &model.weapons.firstFruit << std::endl;

    std::cout << "model.weapons.firstFruit.size(): "
        << model.weapons.firstFruit.Get().size() << std::endl;

    AggregateControl control(model);

    std::cout << "control.weapons.firstFruit.size(): "
        << control.weapons.firstFruit.Get().size() << std::endl;

    std::cout << "control.weapons.firstFruit.size(): "
        << control.weapons.firstFruit.Get().size() << std::endl;

    WeaponsObserver weaponsObserver(control.weapons);

    std::cout << "endpoint connected" << std::endl;

    control.airspeedVelocity = 42.0;
    std::cout << "setting passion fruit" << std::endl;
    control.weapons.firstFruit = "passion fruit";
    std::cout << "setting banana" << std::endl;
    control.weapons.secondFruit = "banana";
    std::cout << "setting pointed stick" << std::endl;
    control.weapons.notFruit = "pointed stick";
    std::cout << "setting gps" << std::endl;
    control.gps.time = 1334706453;
    control.gps.latitude = 40.56923581063791;
    control.gps.longitude = -111.63928609736942;
    control.gps.elevation = 3322.0;

    auto plain = model.Get();

    std::cout << "changing firstFruit to apple" << std::endl;
    plain.weapons.firstFruit = "apple";

    std::cout << "changing secondFruit to cherry" << std::endl;
    plain.weapons.secondFruit = "cherry";

    std::cout << "changing notFruit to rock" << std::endl;
    plain.weapons.notFruit = "rock";

    std::cout << "setting change on model" << std::endl;
    model.Set(plain);

    std::cout << fields::DescribeColorized(model.Get(), 0) << std::endl;

    std::cout << "end of program" << std::endl;
}
