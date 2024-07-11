#include <catch2/catch.hpp>
#include <pex/list.h>
#include <pex/group.h>
#include <pex/poly.h>
#include <pex/endpoint.h>
#include <nlohmann/json.hpp>


// #define LOG_POLY_LIST_TESTS


/*
    Create a polymorphic list
*/


struct Foo: public pex::poly::PolyBase<nlohmann::json, Foo>
{
    static constexpr auto polyTypeName = "Foo";
};

static_assert(pex::poly::detail::IsCompatibleBase<Foo>);


// Make our own base class
class Aircraft: public pex::poly::PolyBase<nlohmann::json, Aircraft>
{
public:
    using Json = nlohmann::json;

    virtual void SayHello() const
    {
        std::cout << "I am default SayHello()" << std::endl;
    }

    static constexpr auto polyTypeName = "Aircraft";
};


using ValueControl = pex::control::Value<pex::model::Value<double>>;


struct AircraftSupers
{
    using ValueBase = Aircraft;

    // Define the abstract control class
    class ControlUserBase
    {
    public:
        virtual ~ControlUserBase() {}
        virtual ValueControl & GetRange() = 0;
        virtual ValueControl & GetMaximumAltitude() = 0;
    };

};


struct CommonTemplates
{
    using Supers = AircraftSupers;

    template<typename Base>
    class Model: public Base
    {
    public:
        using Base::Base;

        static constexpr bool isAircraftCustom = true;

        Model()
            :
            Base()
        {

        }
    };

    // Implement the abstract control functions.
    template<typename Base>
    class Control: public Base
    {
        static_assert(
            std::is_same_v
            <
                ValueControl &,
                decltype(std::declval<Base>().GetRange())
            >);

    public:
        using Base::Base;

        ValueControl & GetRange() override
        {
            return this->range;
        }

        ValueControl & GetMaximumAltitude() override
        {
            return this->maximumAltitude;
        }
    };
};


template<typename T>
class FixedWingFields
{
public:
    static constexpr auto fields = std::make_tuple(
        fields::Field(&T::maximumAltitude, "maximumAltitude"),
        fields::Field(&T::range, "range"),
        fields::Field(&T::wingspan, "wingspan"));
};


struct FixedWingTemplates: public CommonTemplates
{
    template<template<typename> typename T>
    class Template
    {
    public:
        T<double> maximumAltitude;
        T<double> range;
        T<double> wingspan;

        static constexpr auto fields = FixedWingFields<Template>::fields;
        static constexpr auto fieldsTypeName = "FixedWing";
    };

    template<typename Base>
    class Derived: public Base
    {
    public:
        using Base::Base;

        void SayHello() const override
        {
            std::cout << "Hello, I am " << this->GetTypeName()
                << " wingspan: " << this->wingspan << std::endl;
        }
    };
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


struct RotorWingTemplates: public CommonTemplates
{
    template<template<typename> typename T>
    class Template
    {
    public:
        T<double> maximumAltitude;
        T<double> range;
        T<double> rotorRadius;

        static constexpr auto fields = RotorWingFields<Template>::fields;

        static constexpr auto fieldsTypeName = "RotorWing";
    };

    template<typename Base>
    class Derived: public Base
    {
    public:
        using Base::Base;

        void SayHello() const override
        {
            std::cout << "Hello, I am " << this->GetTypeName()
                << " rotorRadius: " << this->rotorRadius << std::endl;
        }
    };
};


using AircraftValue = pex::poly::Value<Aircraft>;


static_assert(pex::detail::HasControlUserBase<AircraftSupers>);

static_assert(
    pex::detail::HasControlTemplate
    <
        FixedWingTemplates,
        FixedWingTemplates::template Template<pex::Identity>
    >);

static_assert(
    pex::detail::HasControlTemplate
    <
        RotorWingTemplates,
        RotorWingTemplates::template Template<pex::Identity>
    >);

static_assert(
    pex::detail::HasModelTemplate
    <
        FixedWingTemplates,
        FixedWingTemplates::template Template<pex::Identity>
    >);

static_assert(
    pex::detail::HasModelTemplate
    <
        RotorWingTemplates,
        RotorWingTemplates::template Template<pex::Identity>
    >);


using RotorWingPoly =
    pex::poly::Poly<RotorWingFields, RotorWingTemplates>;

using RotorWing = typename RotorWingPoly::Derived;
using RotorWingValue = typename RotorWingPoly::PolyValue;


using FixedWingPoly =
    pex::poly::Poly<FixedWingFields, FixedWingTemplates>;

using FixedWing = typename FixedWingPoly::Derived;
using FixedWingValue = typename FixedWingPoly::PolyValue;
using FixedWingControl = typename FixedWingPoly::Control;


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
    T<pex::MakePolyList<AircraftSupers>> aircraft;

    static constexpr auto fields = AirportFields<AirportTemplate>::fields;
    static constexpr auto fieldsTypeName = "Airport";
};


using AirportGroup = pex::Group<AirportFields, AirportTemplate>;
using Airport = typename AirportGroup::Plain;
using Model = typename AirportGroup::Model;
using AirportControl = typename AirportGroup::Control;

template<typename T, typename = void>
struct IsAircraftCustom_: std::false_type {};

template<typename T>
struct IsAircraftCustom_<T, std::enable_if_t<T::isAircraftCustom>>
    : std::true_type {};

template<typename T>
inline constexpr bool IsAircraftCustom = IsAircraftCustom_<T>::value;


static_assert(IsAircraftCustom<typename RotorWingPoly::Model>);
static_assert(IsAircraftCustom<typename FixedWingPoly::Model>);


DECLARE_EQUALITY_OPERATORS(Airport)


TEST_CASE("List of polymorphic values", "[poly]")
{
    Model model;
    AirportControl control(model);

    control.aircraft.Append(RotorWingValue({10000, 175, 25}));
    control.aircraft.Append(RotorWingValue({15000, 300, 34}));
    control.aircraft.Append(FixedWingValue({20000, 800, 50}));
    control.aircraft.Append(FixedWingValue({60000, 7000, 150}));

    REQUIRE(model.aircraft.count.Get() == 4);

    FixedWingControl someControl(model.aircraft[3]);
    FixedWingControl anotherControl(control.aircraft[3]);

    someControl.wingspan.Set(151);
    REQUIRE(anotherControl.wingspan.Get() == 151);

#ifdef LOG_POLY_LIST_TESTS
    std::cout << fields::DescribeColorized(model.Get(), 1) << std::endl;
#endif

    auto fixedWing = control.aircraft[2].Get().RequireDerived<FixedWing>();

    REQUIRE(fixedWing.wingspan == 50);
}


TEST_CASE("List of polymorphic values can be unstructured", "[poly]")
{
    Model model;
    AirportControl control(model);

    control.aircraft.Append(RotorWingValue({10000, 175, 25}));
    control.aircraft.Append(FixedWingValue({20000, 800, 50}));
    control.aircraft.Append(FixedWingValue({60000, 7000, 150}));
    control.aircraft.Append(RotorWingValue({15000, 300, 34}));

    REQUIRE(model.aircraft.count.Get() == 4);

    auto unstructured = fields::Unstructure<nlohmann::json>(model.Get());

#ifdef LOG_POLY_LIST_TESTS
    std::cout << "\nunstructured:\n" << std::setw(4) << unstructured
        << std::endl;
#endif

    auto asString = unstructured.dump(4);

#ifdef LOG_POLY_LIST_TESTS
    std::cout << "asString:\n" << asString << std::endl;
#endif

    auto recoveredUnstructured = nlohmann::json::parse(asString);
    auto recovered = fields::Structure<Airport>(recoveredUnstructured);

#ifdef LOG_POLY_LIST_TESTS
    std::cout << "recovered\n" << fields::DescribeColorized(recovered, 1)
        << std::endl;
#endif

    REQUIRE(recovered == model.Get());

#ifdef LOG_POLY_LIST_TESTS
    std::cout << "aircraft count: " << control.aircraft.count.Get()
        << std::endl;

    for (auto &craft: control.aircraft)
    {
        craft.Get().GetValueBase()->SayHello();
    }
#endif
}


using TestListControl =
    pex::control::List
    <
        pex::model::List<pex::poly::Model<AircraftSupers>, 0>,
        pex::poly::Control<AircraftSupers>
    >;

static_assert(pex::IsListControl<TestListControl>);

using TestControl = pex::poly::Control<AircraftSupers>;

static_assert(pex::IsControl<TestControl>);

using SelectedTestControl = pex::detail::ConnectableSelector<TestControl>;

static_assert(std::is_same_v<TestControl, SelectedTestControl>);


class AircraftObserver
{
public:
    using AircraftListControl = decltype(AirportControl::aircraft);
    static_assert(pex::IsListControl<AircraftListControl>);

    using AircraftEndpoint =
        pex::Endpoint<AircraftObserver, AircraftListControl>;

    using AircraftList = typename AircraftListControl::Type;

    AircraftObserver(AircraftListControl aircraftListControl)
        :
        endpoint_(this, aircraftListControl, &AircraftObserver::OnAircraft_),
        aircraftList_(aircraftListControl.Get()),
        notificationCount_()
    {

    }

    void OnAircraft_(const AircraftList &aircraft)
    {
        this->aircraftList_ = aircraft;
        ++this->notificationCount_;
    }

    size_t GetNotificationCount() const
    {
        return this->notificationCount_;
    }

    const AircraftList & GetAircraft() const
    {
        return this->aircraftList_;
    }

    bool operator==(const AircraftList &aircraftList) const
    {
        if (aircraftList.size() != this->aircraftList_.size())
        {
            return false;
        }

        for (size_t i = 0; i < aircraftList.size(); ++i)
        {
            if (aircraftList[i] != this->aircraftList_[i])
            {
                return false;
            }
        }

        return true;
    }

private:
    AircraftEndpoint endpoint_;
    AircraftList aircraftList_;
    size_t notificationCount_;
};


class AirportObserver
{
public:
    using AirportEndpoint =
        pex::Endpoint<AirportObserver, AirportControl>;

    AirportObserver(AirportControl airportControl)
        :
        endpoint_(this, airportControl, &AirportObserver::OnAirport_),
        airport_(airportControl.Get()),
        notificationCount_()
    {

    }

    void OnAirport_(const Airport &airport)
    {
        this->airport_ = airport;
        ++this->notificationCount_;
    }

    size_t GetNotificationCount() const
    {
        return this->notificationCount_;
    }

    const Airport & GetAirport() const
    {
        return this->airport_;
    }

private:
    AirportEndpoint endpoint_;
    Airport airport_;
    size_t notificationCount_;
};


TEST_CASE("Poly list of groups implements virtual bases.", "[List]")
{
    Model model;
    AirportControl control(model);
    AircraftObserver observer(control.aircraft);

    auto values = GENERATE(
        take(
            3,
            chunk(
                15,
                random(-1000.0, 1000.0))));

    REQUIRE(observer.GetNotificationCount() == 0);

    control.aircraft.Append(
        RotorWingValue({values.at(0), values.at(1), values.at(2)}));

    REQUIRE(observer.GetNotificationCount() == 1);

    control.aircraft.Append(
        FixedWingValue({values.at(3), values.at(4), values.at(5)}));

    control.aircraft.Append(
        FixedWingValue({values.at(6), values.at(7), values.at(8)}));

    control.aircraft.Append(
        RotorWingValue({values.at(9), values.at(10), values.at(11)}));

    AirportObserver airportObserver(control);
    REQUIRE(airportObserver.GetNotificationCount() == 0);

    control.aircraft.Append(
        RotorWingValue({values.at(12), values.at(13), values.at(14)}));

    REQUIRE(airportObserver.GetNotificationCount() == 1);

    control.aircraft[2].GetVirtual()->GetRange().Set(42.0);

    REQUIRE(airportObserver.GetNotificationCount() == 2);

    REQUIRE(observer.GetNotificationCount() == 6);

    REQUIRE(
        model.aircraft[2].Get().RequireDerived<FixedWing>().range == 42.0);

    REQUIRE(
        observer.GetAircraft()[2].RequireDerived<FixedWing>().range == 42.0);

    auto &airport = airportObserver.GetAirport();

    REQUIRE(airport.aircraft.size() == 5);
    REQUIRE(airport.aircraft[2].RequireDerived<FixedWing>().range == 42.0);

    auto aircraft = model.aircraft[2].Get().RequireDerived<FixedWing>();
    aircraft.range = 43.0;
    control.aircraft[2].GetVirtual()->SetValue(aircraft);

    REQUIRE(airportObserver.GetNotificationCount() == 3);

    REQUIRE(
        airportObserver.GetAirport().aircraft[2]
            .RequireDerived<FixedWing>().range
        == 43.0);
}


TEST_CASE("Poly list is observed after going to size 0.", "[List]")
{
    Model model;
    AirportControl control(model);

    AircraftObserver aircraftObserver(control.aircraft);
    REQUIRE(aircraftObserver.GetNotificationCount() == 0);

    AirportObserver airportObserver(control);
    REQUIRE(airportObserver.GetNotificationCount() == 0);

    auto values = GENERATE(
        take(
            3,
            chunk(
                6,
                random(-1000.0, 1000.0))));


    control.aircraft.Append(
        RotorWingValue({values.at(0), values.at(1), values.at(2)}));

    REQUIRE(aircraftObserver.GetNotificationCount() == 1);
    REQUIRE(airportObserver.GetNotificationCount() == 1);

    control.aircraft.count.Set(0);

    REQUIRE(aircraftObserver.GetNotificationCount() == 2);
    REQUIRE(airportObserver.GetNotificationCount() == 2);

    control.aircraft.Append(
        FixedWingValue({values.at(3), values.at(4), values.at(5)}));

    REQUIRE(aircraftObserver.GetNotificationCount() == 3);
    REQUIRE(airportObserver.GetNotificationCount() == 3);

    control.aircraft[0].GetVirtual()->GetRange().Set(42.0);

    REQUIRE(aircraftObserver.GetNotificationCount() == 4);
    REQUIRE(airportObserver.GetNotificationCount() == 4);

    REQUIRE(
        model.aircraft[0].Get().RequireDerived<FixedWing>().range == 42.0);

#if 0
    REQUIRE(
        aircraftObserver.GetAircraft()[0]
            .RequireDerived<FixedWing>().range
        == 42.0);
#endif

    auto &airport = airportObserver.GetAirport();

    REQUIRE(airport.aircraft.size() == 1);
    REQUIRE(airport.aircraft[0].RequireDerived<FixedWing>().range == 42.0);

    auto aircraft = model.aircraft[0].Get().RequireDerived<FixedWing>();
    aircraft.range = 43.0;
    control.aircraft[0].GetVirtual()->SetValue(aircraft);

    REQUIRE(aircraftObserver.GetNotificationCount() == 5);

    REQUIRE(
        airportObserver.GetAirport().aircraft[0]
            .RequireDerived<FixedWing>().range
        == 43.0);

    REQUIRE(airportObserver.GetNotificationCount() == 5);
}


#if 1
template<typename T>
struct SinglePolyFields
{
    static constexpr auto fields = std::make_tuple(
        fields::Field(&T::fixedWing, "fixedWing"),
        fields::Field(&T::rotorWing, "rotorWing"));
};


template<template<typename> typename T>
struct SinglePolyTemplate
{
    T<FixedWingPoly> fixedWing;
    T<RotorWingPoly> rotorWing;

    static constexpr auto fields =
        SinglePolyFields<SinglePolyTemplate>::fields;
};


using SinglePoly = pex::Group<SinglePolyFields, SinglePolyTemplate>;
using SinglePolyModel = typename SinglePoly::Model;
using SinglePolyControl = typename SinglePoly::Control;
using SinglePolyPlain = typename SinglePoly::Plain;


TEST_CASE("Use Poly in a pex::Group", "[poly]")
{
    SinglePolyModel model;
    SinglePolyControl control(model);

    auto values = GENERATE(
        take(
            3,
            chunk(
                6,
                random(-1000.0, 1000.0))));

    auto rotorWing =
        RotorWingValue({values.at(0), values.at(1), values.at(2)});

    control.rotorWing.SetValue(rotorWing);

    auto fixedWing =
        FixedWingValue({values.at(0), values.at(1), values.at(2)});

    control.fixedWing.SetValue(fixedWing);

    // std::cout << fields::Describe(model.Get(), 1) << std::endl;

    REQUIRE(fixedWing == model.fixedWing.GetValue());
    REQUIRE(rotorWing == model.rotorWing.GetValue());
}
#endif
