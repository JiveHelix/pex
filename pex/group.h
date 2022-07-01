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

// Create the Plain-old-data structure, Model, and Control types
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

    struct Model:
        public Template<pex::ModelSelector>,
        public Accessors
        <
            Plain,
            Fields,
            Template,
            ModelSelector,
            Model,
            detail::NotifyMany<ModelConnection, GetAndSetTag>
        >
    {
    public:
        using Plain = typename Group::Plain;
        using Type = Plain;

        template<typename T>
        using Pex = pex::ModelSelector<T>;

        Model()
        {

        }

        Model(const Plain &plain)
        {
            this->Set(plain);
        }

        Model(const Model &) = delete;
        Model(Model &&) = delete;
        Model & operator=(const Model &) = delete;
        Model & operator=(Model &&) = delete;

        using Callable = typename ModelConnection::Callable;

        void Connect(void * observer, Callable callable)
        {
            this->Connect_(observer, callable);
        }
    };


    template<typename Observer>
    using ControlConnection = ValueConnection<Observer, Plain, NoFilter>;

    template<typename Observer>
    using ControlTemplate =
        Template<pex::ControlSelector<Observer>::template Type>;

    template<typename Observer, typename Derived>
    using ControlBase = Accessors
        <
            Plain,
            Fields,
            Template,
            ControlSelector<Observer>::template Type,
            Derived,
            detail::NotifyMany<ControlConnection<Observer>, GetAndSetTag>
        >;

    template<typename Observer>
    struct Control:
        public ControlTemplate<Observer>,
        public ControlBase<Observer, Control<Observer>>
    {
        using Base = ControlBase<Observer, Control<Observer>>;

        using Plain = typename Group::Plain;
        using Type = Plain;

        template<typename T>
        using Pex =
            typename pex::ControlSelector<Observer>::template Type<T>;

        using Callable = typename ControlConnection<Observer>::Callable;

        Control()
        {

        }

        Control(Model &model)
        {
            fields::AssignConvert<Fields>(*this, model);
        }

        Control(const Control &other)
            :
            Base(other)
        {
            fields::Assign<Fields>(*this, other);
        }

        Control & operator=(const Control &other)
        {
            this->Base::operator=(other);
            fields::Assign<Fields>(*this, other);

            return *this;
        }

        template<typename Other>
        Control(const Control<Other> &other)
            :
            Base(other)
        {
            fields::AssignConvert<Fields>(*this, other);
        }

        template<typename Other>
        Control & operator=(const Control<Other> &other)
        {
            this->Base::operator=(other);
            fields::AssignConvert<Fields>(*this, other);
            return *this;
        }

        ~Control()
        {
#if ENABLE_PEX_LOG
            if (!this->connections_.empty())
            {
                for (auto &connection: this->connections_)
                {
                    PEX_LOG(
                        "Warning: ",
                        connection.GetObserver(),
                        " is still connected to Control group ",
                        this);
                }
            }
#endif
        }

        void Connect(Observer *observer, Callable callable)
        {
            this->Connect_(observer, callable);
        }

        bool HasModel() const
        {
            bool result = true;

            auto modelChecker = [this, &result](auto field)
            {
                if (result)
                {
                    result = (this->*(field.member)).HasModel();
                }
            };

            jive::ForEach(Fields<Control>::fields, modelChecker);

            return result;
        }
    };

    template<typename Observer>
    using TerminusConnection = ValueConnection<Observer, Plain, NoFilter>;

    template<typename Observer>
    using TerminusTemplate =
        Template<pex::TerminusSelector<Observer>::template Type>;

    template<typename Observer, typename Derived>
    using TerminusBase = Accessors
        <
            Plain,
            Fields,
            Template,
            TerminusSelector<Observer>::template Type,
            Derived,
            detail::NotifyMany<TerminusConnection<Observer>, GetAndSetTag>
        >;

    template<typename Observer>
    struct Terminus:
        public TerminusTemplate<Observer>,
        public TerminusBase<Observer, Terminus<Observer>>
    {
        using Plain = typename Group::Plain;
        using Base = TerminusBase<Observer, Terminus<Observer>>;

        template<typename T>
        using Pex =
            typename pex::TerminusSelector<Observer>::template Type<T>;

        using Callable = typename TerminusConnection<Observer>::Callable;

        Terminus()
        {

        }

        Terminus(Observer *observer, const Control<void> &control)
            :
            observer_(observer)
        {
            PEX_LOG("Terminus ctor");

            assert(control.HasModel());

            auto initializer = [this, observer, &control](
                auto terminusField,
                auto controlField)
            {
                using MemberType = typename std::remove_reference_t
                    <
                        decltype(this->*(terminusField.member))
                    >;

                this->*(terminusField.member) =
                    MemberType(observer, control.*(controlField.member));
            };

            jive::ZipApply(
                initializer,
                Fields<Terminus>::fields,
                Fields<Control<void>>::fields);

            auto logger = [this](const auto &terminusField) -> void
            {
                PEX_LOG(
                    "Terminus.",
                    terminusField.name,
                    ": ",
                    &(this->*(terminusField.member)));
            };

            jive::ForEach(Fields<Terminus>::fields, logger);
        }

        Terminus(Observer *observer, Model &model)
        {
            *this = Terminus(observer, Control<void>(model));
        }

        Terminus(const Terminus &) = delete;

        Terminus(Terminus &&other)
            :
            Base(std::move(other)),
            observer_(std::move(other.observer_))
        {
            auto initializer = [this, &other](auto field)
            {
                this->*(field.member) = std::move(other.*(field.member));
            };

            jive::ForEach(
                Fields<Terminus>::fields,
                initializer);
        }

        Terminus & operator=(const Terminus &) = delete;

        Terminus & operator=(Terminus &&other)
        {
            this->Base::operator=(std::move(other));
            this->observer_ = std::move(other.observer_);

            auto initializer = [this, &other](auto field)
            {
                this->*(field.member) = std::move(other.*(field.member));
            };

            jive::ForEach(
                Fields<Terminus>::fields,
                initializer);

            return *this;
        }

        void Connect(Callable callable)
        {
            this->Connect_(this->observer_, callable);
        }

        ~Terminus()
        {
            PEX_LOG(this);
            this->Disconnect();
        }

        void Disconnect()
        {
            this->ClearConnections_();
        }

        bool HasModel() const
        {
            bool result = true;

            auto modelChecker = [this, &result](auto field)
            {
                if (result)
                {
                    result = (this->*(field.member)).HasModel();
                }
            };

            jive::ForEach(Fields<Terminus>::fields, modelChecker);

            return result;
        }
    
    private:
        Observer * observer_;
    };
};


} // end namespace pex
