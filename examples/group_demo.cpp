#include <iostream>
#include <pex/group.h>
#include <fields/fields.h>


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

template<typename Observer>
using WeaponsControl = typename WeaponsGroup::Control<Observer>;




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

template<typename Observer>
using GpsControl = typename GpsGroup::Control<Observer>;


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
using AggregateControl = typename AggregateGroup::Control<void>;


int main()
{
    AggregateModel model;
    AggregateControl control(model);

    control.airspeedVelocity = 42.0;
    control.weapons.firstFruit = "passion fruit";
    control.weapons.secondFruit = "banana";
    control.weapons.notFruit = "pointed stick";
    control.gps.time = 1334706453;
    control.gps.latitude = 40.56923581063791;
    control.gps.longitude = -111.63928609736942;
    control.gps.elevation = 3322.0;
    
    auto plain = model.Get();
    plain.weapons.secondFruit = "cherry";
    model.Set(plain);

    std::cout << fields::DescribeColorized(model.Get(), 0) << std::endl;
}
