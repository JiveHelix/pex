#include <iostream>
#include <pex/group.h>
#include <fields/fields.h>
#include <pex/endpoint.h>
#include <pex/identity.h>


template<template<typename> typename T>
struct WeaponsSchema
{
    T<std::string> firstFruit;
    T<std::string> secondFruit;
    T<std::string> notFruit;

    static constexpr auto fieldsTypeName = "Weapons";
};


using WeaponsGroup = pex::Group<WeaponsSchema>;
using WeaponsPlain = typename WeaponsGroup::Plain;
using WeaponsModel = typename WeaponsGroup::Model;

using WeaponsControl = typename WeaponsGroup::Control<WeaponsModel>;


template<template<typename> typename T>
struct GpsSchema
{
    T<int64_t> time;
    T<double> latitude;
    T<double> longitude;
    T<double> elevation;

    static constexpr auto fieldsTypeName = "Gps";
};


using GpsGroup = pex::Group<GpsSchema>;
using GpsPlain = typename GpsGroup::Plain;
using GpsModel = typename GpsGroup::Model;

using GpsControl = typename GpsGroup::Control<GpsModel>;


inline GpsPlain DefaultGps()
{
    return GpsPlain{
        1334706453,
        40.56923581063791,
        -111.63928609736942,
        3322.0};
}


template<template<typename> typename T>
struct CombinedSchema
{
    T<double> airspeedVelocity;
    T<WeaponsGroup> weapons;
    T<GpsGroup> gps;

    static constexpr auto fieldsTypeName = "Combined";
};


using CombinedGroup = pex::Group<CombinedSchema>;
using CombinedModel = typename CombinedGroup::Model;
using CombinedControl = typename CombinedGroup::Control<CombinedModel>;


class WeaponsObserver
{
public:
    WeaponsObserver(const WeaponsControl &control)
        :
        endpoint_(
            PEX_THIS("WeaponsObserver"),
            control,
            &WeaponsObserver::OnWeapons)
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
    CombinedModel model;
    CombinedControl control(model);

    WeaponsObserver weaponsObserver(control.weapons);

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
}
