#include <iostream>

#include <pex/group.h>
#include <fields/fields.h>


template<typename T>
struct PositionFields
{
    static constexpr auto fields = std::make_tuple(
        fields::Field(&T::x, "x"),
        fields::Field(&T::y, "y"),
        fields::Field(&T::z, "z"));
};


template<template<typename> typename T>
struct PositionTemplate
{
    T<double> x;
    T<double> y;
    T<double> z;

    static constexpr auto fields = PositionFields<PositionTemplate>::fields;
    static constexpr auto fieldsTypeName = "Position";
};


template<typename T>
struct RotationFields
{
    static constexpr auto fields = std::make_tuple(
        fields::Field(&T::alpha, "alpha"),
        fields::Field(&T::beta, "beta"),
        fields::Field(&T::gamma, "gamma"));
};


using pex::Limit;


template<template<typename> typename T>
struct RotationTemplate
{
    T<pex::MakeRange<double, Limit<-90>, Limit<90>>> alpha;
    T<pex::MakeRange<double, Limit<-180>, Limit<180>>> beta;
    T<pex::MakeRange<double, Limit<-180>, Limit<180>>> gamma;

    static constexpr auto fields = RotationFields<RotationTemplate>::fields;
    static constexpr auto fieldsTypeName = "Rotation";
};


using PositionGroup = pex::Group<PositionFields, PositionTemplate>;
using RotationGroup = pex::Group<RotationFields, RotationTemplate>;


template<typename T>
struct PoseFields
{
    static constexpr auto fields = std::make_tuple(
        fields::Field(&T::position, "position"),
        fields::Field(&T::rotation, "rotation"));
};


template<template<typename> typename T>
struct PoseTemplate
{
    T<pex::MakeGroup<PositionGroup>> position;
    T<pex::MakeGroup<RotationGroup>> rotation;

    static constexpr auto fields = PoseFields<PoseTemplate>::fields;
    static constexpr auto fieldsTypeName = "Pose";
};


using PoseGroup = pex::Group<PoseFields, PoseTemplate>;
using Pose = typename PoseGroup::Plain;
using PoseModel = typename PoseGroup::Model;
using PoseControl = typename PoseGroup::Control<void>;
using PoseTerminus = typename PoseGroup::Terminus<void>;


void OnPose(void *, const Pose &pose)
{
    std::cout << "OnPose: " << fields::DescribeColorized(pose, 1)
              << std::endl;
}


int main()
{
    std::cout << "begin program" << std::endl;

    PoseModel model;
    PoseControl control(model);
    PoseTerminus terminus(nullptr, model);

    terminus.Connect(&OnPose);

    std::cout << "setting position.x = 42" << std::endl;
    control.position.x = 42.0;

    std::cout << "setting position.y = 99" << std::endl;
    control.position.y = 99.0;

    std::cout << "setting position.z = -42" << std::endl;
    control.position.z = -42.0;

    std::cout << "setting rotation.alpha = 110" << std::endl;
    control.rotation.alpha = 110.0;

    std::cout << "setting rotation.beta = -181" << std::endl;
    control.rotation.beta = -181.0;

    std::cout << "setting rotation.gamma = 300" << std::endl;
    control.rotation.gamma = 300.0;

    auto plain = model.Get();

    plain.position.x = 1.0;
    plain.position.y = 2.0;
    plain.position.z = 3.0;

    std::cout << "Changing the entire struct on the terminus." << std::endl;
    terminus.Set(plain);

    std::cout << fields::DescribeColorized(model.Get(), 0) << std::endl;

    std::cout << "end program" << std::endl;
}
