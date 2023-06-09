#pragma once


#include <fields/fields.h>

#include "pex/accessors.h"
#include "pex/traits.h"


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
    template<typename> typename Fields_,
    template<template<typename> typename> typename Template_,
    typename Plain_ = void
>
struct Group
{
    template<typename T>
    using Fields = Fields_<T>;

    template<template<typename> typename T>
    using Template = Template_<T>;

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

    template<template<typename> typename Selector>
    using DeferGroup = DeferGroup<Fields, Template, Selector>;

    using DeferModel = DeferGroup<ModelSelector>;

    template<typename Observer>
    using DeferControl = DeferGroup<ControlSelector<Observer>::template Type>;

    template<typename Observer>
    using DeferTerminus = DeferGroup<TerminusSelector<Observer>::template Type>;

    using ModelConnection = ValueConnection<void, Plain, NoFilter>;

    template<typename Derived>
    using ModelAccessors = Accessors
        <
            Plain,
            Fields,
            Template,
            ModelSelector,
            Derived,
            detail::NotifyMany<ModelConnection, GetAndSetTag>
        >;

    struct Model:
        public MuteOwner,
        public MuteGroup<Model>,
        public Template<ModelSelector>,
        public ModelAccessors<Model>
    {
    public:
        using Plain = typename Group::Plain;
        using Type = Plain;
        using Defer = DeferModel;

        template<typename T>
        using Pex = pex::ModelSelector<T>;

        Model()
            :
            MuteOwner(),
            MuteGroup<Model>(this->GetMuteControl())
        {
            if constexpr (HasDefault<Plain>)
            {
                this->Set(Plain::Default());
            }
        }

        Model(const Plain &plain)
            :
            MuteOwner(),
            MuteGroup<Model>(this->GetMuteControl())
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

        bool HasModel() const { return true; }
    };

    template<typename Observer>
    using ControlConnection = ValueConnection<Observer, Plain, NoFilter>;

    template<typename Observer>
    using ControlTemplate =
        Template<pex::ControlSelector<Observer>::template Type>;

    template<typename Observer, typename Derived>
    using ControlAccessors = Accessors
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
        public MuteGroup<Control<Observer>>,
        public ControlTemplate<Observer>,
        public ControlAccessors<Observer, Control<Observer>>
    {
        using AccessorsBase = ControlAccessors<Observer, Control<Observer>>;

        using Plain = typename Group::Plain;
        using Type = Plain;

        using Upstream = Model;
        using Defer = DeferControl<Observer>;

        // UpstreamType could be the type returned by a filter.
        // Filters have not been implemented by this class, so it remains the
        // same as the Type.
        using UpstreamType = Plain;
        using Filter = NoFilter;

        static constexpr bool isPexCopyable = true;

        template<typename T>
        using Pex =
            typename pex::ControlSelector<Observer>::template Type<T>;

        using Callable = typename ControlConnection<Observer>::Callable;

        Control()
            :
            MuteGroup<Control<Observer>>(),
            AccessorsBase()
        {

        }

        Control(Model &model)
            :
            MuteGroup<Control<Observer>>(model),
            AccessorsBase()
        {
            fields::AssignConvert<Fields>(*this, model);
        }

        Control(const Control &other)
            :
            MuteGroup<Control<Observer>>(other),
            AccessorsBase()
        {
            fields::Assign<Fields>(*this, other);
        }

        Control & operator=(const Control &other)
        {
            this->MuteGroup<Control<Observer>>::operator=(other);
            this->ResetAccessors_();
            fields::Assign<Fields>(*this, other);

            return *this;
        }

        template<typename Other>
        Control(const Control<Other> &other)
            :
            MuteGroup<Control<Observer>>(other),
            AccessorsBase()
        {
            fields::AssignConvert<Fields>(*this, other);
        }

        template<typename Other>
        Control & operator=(const Control<Other> &other)
        {
            this->MuteGroup<Control<Observer>>::operator=(other);
            this->ResetAccessors_();
            fields::AssignConvert<Fields>(*this, other);
            return *this;
        }

        ~Control()
        {
#ifdef ENABLE_PEX_LOG
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
            return pex::HasModel<Fields>(*this);
        }
    };

    template<typename Observer>
    using TerminusConnection = ValueConnection<Observer, Plain, NoFilter>;

    template<typename Observer>
    using TerminusTemplate =
        Template<pex::TerminusSelector<Observer>::template Type>;

    template<typename Observer, typename Derived>
    using TerminusAccessors = Accessors
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
        public MuteGroup<Terminus<Observer>>,
        public TerminusTemplate<Observer>,
        public TerminusAccessors<Observer, Terminus<Observer>>
    {
        using Plain = typename Group::Plain;
        using Type = Plain;
        using AccessorsBase = TerminusAccessors<Observer, Terminus<Observer>>;
        using Pex = Control<Observer>;
        using Callable = typename TerminusConnection<Observer>::Callable;
        using Defer = DeferTerminus<Observer>;

        Terminus() = default;

        Terminus(Observer *observer, const Control<void> &control)
            :
            MuteGroup<Terminus<Observer>>(control),
            observer_(observer)
        {
            PEX_LOG("Terminus construct from Control with ", observer);
            InitializeTerminus<Fields>(observer, *this, control);
        }

        Terminus(Observer *observer, Model &model)
            :
            MuteGroup<Terminus<Observer>>(model),
            observer_(observer)
        {
            PEX_LOG("Terminus construct from Model with ", observer);
            this->Assign(observer, Terminus(observer, Control<void>(model)));
        }


        Terminus(const Terminus &) = delete;
        Terminus(Terminus &&) = delete;
        Terminus & operator=(const Terminus &) = delete;
        Terminus & operator=(Terminus &&) = delete;

        template<typename U>
        friend struct Terminus;

        // Copy construct
        Terminus(Observer *observer, const Terminus &other)
            :
            MuteGroup<Terminus<Observer>>(other),
            AccessorsBase(),
            observer_(observer)
        {
            PEX_LOG("Terminus copy construct observer: ", observer);
            CopyTerminus<Fields>(observer, *this, other);
        }

        // Copy construct
        template<typename Other>
        Terminus(Observer *observer, const Terminus<Other> &other)
            :
            MuteGroup<Terminus<Observer>>(other),
            AccessorsBase(),
            observer_(observer)
        {
            PEX_LOG("Terminus copy construct observer: ", observer);
            CopyTerminus<Fields>(observer, *this, other);
        }

        // Move construct
        Terminus(Observer *observer, Terminus &&other)
            :
            MuteGroup<Terminus<Observer>>(other),
            AccessorsBase(),
            observer_(observer)
        {
            PEX_LOG("Terminus move construct observer: ", observer);
            MoveTerminus<Fields>(observer, *this, other);
        }

        // Move construct
        template<typename Other>
        Terminus(Observer *observer, Terminus<Other> &&other)
            :
            MuteGroup<Terminus<Observer>>(other),
            AccessorsBase(),
            observer_(observer)
        {
            PEX_LOG("Terminus move construct observer: ", observer);
            MoveTerminus<Fields>(observer, *this, other);
        }

        // Copy assign
        Terminus & Assign(Observer *observer, const Terminus &other)
        {
            PEX_LOG("Terminus copy assign observer: ", observer);
            this->MuteGroup<Terminus<Observer>>::operator=(other);
            this->ResetAccessors_();
            this->observer_ = observer;

            CopyTerminus<Fields>(observer, *this, other);

            return *this;
        }

        // Copy assign
        template<typename Other>
        Terminus & Assign(
            Observer *observer,
            const Terminus<Other> &other)
        {
            PEX_LOG("Terminus copy assign observer: ", observer);
            this->MuteGroup<Terminus<Observer>>::operator=(other);
            this->ResetAccessors_();
            this->observer_ = observer;

            CopyTerminus<Fields>(observer, *this, other);

            return *this;
        }

        // Move assign
        Terminus & Assign(Observer *observer, Terminus &&other)
        {
            PEX_LOG("Terminus move assign observer: ", observer);
            this->MuteGroup<Terminus<Observer>>::operator=(other);
            this->ResetAccessors_();
            this->observer_ = observer;

            MoveTerminus<Fields>(observer, *this, other);

            return *this;
        }

        // Move assign
        template<typename Other>
        Terminus & Assign(Observer *observer, Terminus<Other> &&other)
        {
            PEX_LOG("Terminus move assign observer: ", observer);
            this->MuteGroup<Terminus<Observer>>::operator=(other);
            this->ResetAccessors_();
            this->observer_ = observer;

            MoveTerminus<Fields>(observer, *this, other);

            return *this;
        }

        void Connect(Callable callable)
        {
            PEX_LOG(
                "Group::Terminus::Connect with observer: ",
                this->observer_);

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
            return pex::HasModel<Fields>(*this);
        }

        Observer * GetObserver()
        {
            return this->observer_;
        }

        explicit operator Control<void> () const
        {
            Control<void> result;
            fields::AssignConvert<Fields>(result, *this);
            return result;
        }

    private:
        Observer * observer_;
    };

    static DeferModel MakeDefer(Model &model)
    {
        return DeferModel(model);
    }

    template<typename Observer>
    static DeferControl<Observer> MakeDefer(Control<Observer> &control)
    {
        return DeferControl<Observer>(control);
    }

    template<typename Observer>
    static DeferTerminus<Observer> MakeDefer(Terminus<Observer> &terminus)
    {
        return DeferTerminus<Observer>(terminus);
    }
};


} // end namespace pex
