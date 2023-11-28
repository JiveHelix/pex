#include <catch2/catch.hpp>
#include <pex/list.h>
#include <pex/group.h>
#include <pex/poly.h>
#include <pex/poly_group.h>
#include <nlohmann/json.hpp>


/*
    Create a polymorphic list
*/

#if 0
struct Aircraft: public pex::poly::PolyBase<nlohmann::json>
{
    static constexpr auto polyTypeName = "Aircraft";
};
#else

// Make our own base class
class Aircraft
{
public:
    using Json = nlohmann::json;

    virtual ~Aircraft() {}

    virtual std::ostream & Describe(
        std::ostream &outputStream,
        const fields::Style &style,
        int indent) const = 0;

    virtual Json Unstructure() const = 0;
    virtual bool operator==(const Aircraft &) const = 0;

    static constexpr auto polyTypeName = "Aircraft";
};

#endif


template<typename T>
class FixedWingFields
{
public:
    static constexpr auto fields = std::make_tuple(
        fields::Field(&T::maximumAltitude, "maximumAltitude"),
        fields::Field(&T::range, "range"),
        fields::Field(&T::wingspan, "wingspan"));
};


template<template<typename> typename T>
class FixedWingTemplate
{
public:
    T<double> maximumAltitude;
    T<double> range;
    T<double> wingspan;

    static constexpr auto fields = FixedWingFields<FixedWingTemplate>::fields;
    static constexpr auto fieldsTypeName = "FixedWing";
};


template<typename T>
class RotorWingFields
{
public:
    static constexpr auto fields = std::make_tuple(
        fields::Field(&T::maximumAltitude, "maximumAltitude"),
        fields::Field(&T::range, "range"),
        fields::Field(&T::rotorRadius, "rotorRadius"));
};


template<template<typename> typename T>
class RotorWingTemplate
{
public:
    T<double> maximumAltitude;
    T<double> range;
    T<double> rotorRadius;

    static constexpr auto fields = RotorWingFields<RotorWingTemplate>::fields;
    static constexpr auto fieldsTypeName = "RotorWing";
};


using AircraftValue =
    pex::poly::Value<Aircraft, FixedWingTemplate, RotorWingTemplate>;


using RotorWingGroup =
    pex::poly::PolyGroup
    <
        RotorWingFields,
        RotorWingTemplate,
        AircraftValue
    >;

using RotorWing = typename RotorWingGroup::Derived;
using RotorWingValue = typename RotorWingGroup::PolyValue;


using FixedWingGroup =
    pex::poly::PolyGroup
    <
        FixedWingFields,
        FixedWingTemplate,
        AircraftValue
    >;

using FixedWing = typename FixedWingGroup::Derived;
using FixedWingValue = typename FixedWingGroup::PolyValue;
using FixedWingControl = typename FixedWingGroup::Control;


template<typename T>
struct AirportFields
{
    static constexpr auto fields = std::make_tuple(
        fields::Field(&T::runwayCount, "runwayCount"),
        fields::Field(&T::dailyPassengerCount, "dailyPassengerCount"),
        fields::Field(&T::aircraft, "aircraft"));
};


template<template<typename> typename T>
class AirportTemplate
{
public:
    T<size_t> runwayCount;
    T<size_t> dailyPassengerCount;
    T<pex::MakePolyList<AircraftValue, 0>> aircraft;

    static constexpr auto fields = AirportFields<AirportTemplate>::fields;
    static constexpr auto fieldsTypeName = "Airport";
};


using AirportGroup = pex::Group<AirportFields, AirportTemplate>;
using Airport = typename AirportGroup::Plain;
using Model = typename AirportGroup::Model;
using Control = typename AirportGroup::Control;


DECLARE_EQUALITY_OPERATORS(Airport)


TEST_CASE("List of polymorphic values", "[poly]")
{
    Model model;
    Control control(model);

    control.aircraft.Append(RotorWingValue({10000, 175, 25}));
    control.aircraft.Append(RotorWingValue({15000, 300, 34}));
    control.aircraft.Append(FixedWingValue({20000, 800, 50}));
    control.aircraft.Append(FixedWingValue({60000, 7000, 150}));

    REQUIRE(model.aircraft.count.Get() == 4);

    FixedWingControl someControl(model.aircraft[3]);
    FixedWingControl anotherControl(control.aircraft[3]);

    someControl.wingspan.Set(151);
    REQUIRE(anotherControl.wingspan.Get() == 151);

    // std::cout << fields::DescribeColorized(model.Get(), 1) << std::endl;

    auto fixedWing = control.aircraft[2].Get().RequireDerived<FixedWing>();

    REQUIRE(fixedWing.wingspan == 50);
}


TEST_CASE("List of polymorphic values can be unstructured", "[poly]")
{
    Model model;
    Control control(model);

    control.aircraft.Append(RotorWingValue({10000, 175, 25}));
    control.aircraft.Append(FixedWingValue({20000, 800, 50}));
    control.aircraft.Append(FixedWingValue({60000, 7000, 150}));
    control.aircraft.Append(RotorWingValue({15000, 300, 34}));

    REQUIRE(model.aircraft.count.Get() == 4);

    auto unstructured = fields::Unstructure<nlohmann::json>(model.Get());

    // std::cout << "\nunstructured:\n" << std::setw(4) << unstructured
        // << std::endl;

    auto asString = unstructured.dump();

    // std::cout << "asString:\n" << asString << std::endl;

    auto recoveredUnstructured = nlohmann::json::parse(asString);
    auto recovered = fields::Structure<Airport>(recoveredUnstructured);

    // std::cout << "recovered\n" << fields::DescribeColorized(recovered, 1)
    //     << std::endl;

    REQUIRE(recovered == model.Get());
}
