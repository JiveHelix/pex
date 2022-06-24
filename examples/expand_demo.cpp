
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


struct Player: public PlayerTemplate<pex::Identity>
{
    /*

    std::string name;
    uint16_t age;
    double height;

    */
};


using PlayerModel = pex::model::Value<Player>;
using PlayerControl = pex::control::Value<void, PlayerModel>;

using PlayerExpanded =
    typename pex::Expand<Player, PlayerFields, PlayerTemplate>::Control<void>;

void PlayerDemo()
{
    // Instantiate the model.
    PlayerModel model{};

    // Instantiate and initialize the expanded controls.
    PlayerExpanded expanded((PlayerControl(model)));

    expanded.name.Set("Matthew Stafford");
    expanded.age.Set(34);
    expanded.height.Set(1.905);

    std::cout << fields::DescribeColorized(model.Get(), 1) << std::endl;
}


int main()
{
    PlayerDemo();
}
