#pragma once


#include <fields/fields.h>

#include "pex/accessors.h"
#include "pex/interface.h"


/**
 
// The Group struct uses a Fields type and a Template type to create a POD
// struct and corresponding Model and Control.

// Fields must accept a single template parameter for any type that defines the
// member fields, like this:

template<typename T>
struct GpsFields
{
    static constexpr auto fields = std::make_tuple(
        fields::Field(&T::time, "time"),
        fields::Field(&T::latitude, "latitude"),
        fields::Field(&T::longitude, "longitude"),
        fields::Field(&T::elevation, "elevation"));
};


// The Template type defines the members with template parameter types. This
// allows the definition of the POD struct with pex::Identity, and the Model
// and Control classes.

template<template<typename> typename T>
struct GpsTemplate
{
    T<int64_t> time;
    T<double> latitude;
    T<double> longitude;
    T<double> elevation;

    static constexpr auto fields = GpsFields<MovieTemplate>::fields;
};

// Create the POD, Model, and Control types
using GpsGroup = Group<GpsFields, GpsTemplate>

using Gps = typename GpsGroup::Plain;
using GpsModel = typename GpsGroup::Model;
using GpsControl = typename GpsGroup::Control;

**/


namespace pex
{


template
<
    template<typename> typename Fields,
    template<template<typename> typename> typename Template,
    typename Plain_ = void
>
struct Group
{
    template<typename P, typename = void>
    struct PlainHelper
    {
        using Type = P;
    };

    template<typename P>
    struct PlainHelper
    <
        P,
        std::enable_if_t<std::is_same_v<P, void>>
    >
    {
        struct Plain: public Template<pex::Identity>
        {

        };

        using Type = Plain;
    };
    
    using Plain = typename PlainHelper<Plain_>::Type;
    
    using ModelConnection = ValueConnection<void, Plain, NoFilter>;

    using ModelBase = detail::NotifyMany<ModelConnection, GetAndSetTag>;

    struct Model:
        public Template<pex::ModelSelector>,
        public Accessors<Plain, Model, Fields>,
        public ModelBase
    {
    public:
        using Plain = typename Group::Plain;

        template<typename T>
        using Pex = pex::ModelSelector<T>;

        Model()
            :
            hasConnections_(false)
        {

        }

        Model(const Plain &plain)
            :
            hasConnections_(false)
        {
            this->Set(plain);
        }

        using Callable = typename ModelConnection::Callable;

        void Connect(void * context, Callable callable)
        {
            this->MakeConnections();
            ModelBase::Connect(context, callable);
        }

        void MakeConnections()
        {
            if (this->hasConnections_)
            {
                return;
            }

            auto connector = [this](const auto &modelField) -> void
            {
                using MemberType = typename std::remove_reference_t<
                    decltype(this->*(modelField.member))>::Type;

                (this->*(modelField.member)).Connect(
                    this,
                    &Model::template OnMemberChanged_<MemberType>);
            };

            fields::ForEachField<Model>(connector);
            this->hasConnections_ = true;
        }

        template<typename T>
        static void OnMemberChanged_(void * context, Argument<T>)
        {
            auto self = static_cast<Model *>(context);
            self->Notify_(self->Get());
        }
        
    private:
        bool hasConnections_;
    };

    template<typename Observer>
    using ControlConnection = ValueConnection<Observer, Plain, NoFilter>;

    template<typename Observer>
    using ControlBase =
        detail::NotifyMany<ControlConnection<Observer>, GetAndSetTag>;

    template<typename Observer>
    using ControlTemplate =
        Template<pex::ControlSelector<Observer>::template Type>;

    template<typename Observer>
    struct Control:
        public ControlTemplate<Observer>,
        public Accessors<Plain, Control<Observer>, Fields>,
        public ControlBase<Observer>
    {
        using Plain = typename Group::Plain;

        template<typename T>
        using Pex =
            typename pex::ControlSelector<Observer>::template Type<T>;

        using Callable = typename ControlConnection<Observer>::Callable;

        Control() = default;

        Control(Model &model)
        {
            fields::AssignConvert<Fields>(*this, model);
        }

        template<typename Other>
        Control(const Control<Other> &other)
        {
            fields::AssignConvert<Fields>(*this, other);
        }

        void Connect(Observer * observer)
        {
            auto connector = [this, observer](const auto &controlField) -> void
            {
                using MemberType = typename std::remove_reference_t<
                    decltype(this->*(controlField.member))>::Type;

                (this->*(controlField.member)).Connect(
                    observer,
                    &Observer::template OnMemberChanged<MemberType>);
            };

            fields::ForEachField<Control>(connector);
        }
    };
};


} // end namespace pex
