#include <catch2/catch.hpp>
#include <fields/fields.h>

#include "pex/value.h"
#include "pex/expand.h"
#include "pex/interface.h"


template<typename T>
struct PlayerFields
{
    static constexpr auto fields = std::make_tuple(
        fields::Field(&T::name, "name"),
        fields::Field(&T::age, "age"),
        fields::Field(&T::height, "height"));
};

template<template<typename> typename T>
struct PlayerTemplate
{
    T<std::string> name;
    T<uint16_t> age;
    T<double> height;

    static constexpr auto fields = PlayerFields<PlayerTemplate<T>>::fields;
};


struct Player: public PlayerTemplate<pex::Identity> {};

using PlayerModel = pex::model::Value<Player>;
using PlayerControl = pex::control::Value<void, PlayerModel>;

template<typename T>
using PlayerExpand = pex::Expand<PlayerControl, T>;

struct PlayerExpanded: public PlayerTemplate<PlayerExpand> {};


TEST_CASE("Fan out controls for struct.", "[expand]")
{
    // Instantiate the model.
    PlayerModel model{};

    // Instantiate and initialize the expanded controls.
    PlayerExpanded expanded;
    pex::InitializeExpanded<PlayerFields>(expanded, PlayerControl(model));

    expanded.name.Set("Matthew Stafford");
    expanded.age.Set(34);
    expanded.height.Set(1.905);

    REQUIRE(model.Get().name == "Matthew Stafford");
    REQUIRE(model.Get().age == 34);
    REQUIRE(model.Get().height == Approx(1.905));
}
