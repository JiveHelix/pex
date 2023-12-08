#pragma once


#include <fields/fields.h>

#include "pex/selectors.h"
#include "pex/accessors.h"
#include "pex/traits.h"
#include "pex/detail/mute.h"
#include "pex/detail/aggregate.h"
#include "pex/detail/initialize_terminus.h"


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
    static constexpr bool isGroup = true;

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
    using Type = Plain;

    template<template<typename> typename Selector, typename Upstream>
    using DeferGroup = DeferGroup<Fields, Template, Selector, Upstream>;

    template<template<typename> typename Selector>
    using DeferredGroup = detail::DeferredGroup<Fields, Template, Selector>;


    template<typename Derived>
    using ModelAccessors = GroupAccessors
        <
            Plain,
            Fields,
            Template_,
            ModelSelector,
            Derived
        >;

    struct Control;

    struct Model:
        public Template_<ModelSelector>,
        public detail::MuteOwner,
        public detail::Mute,
        public ModelAccessors<Model>
    {
    public:
        static constexpr bool isGroupModel = true;

        using ControlType = Control;
        using Plain = typename Group::Plain;
        using Type = Plain;
        using Defer = DeferGroup<ModelSelector, Model>;

        template<typename T>
        using Pex = pex::ModelSelector<T>;

        Model()
            :
            Template<ModelSelector>(),
            detail::MuteOwner(),
            detail::Mute(this->GetMuteControl()),
            ModelAccessors<Model>()
        {
            if constexpr (HasDefault<Plain>)
            {
                this->SetWithoutNotify_(Plain::Default());
            }
        }

        Model(const Plain &plain)
            :
            Template<ModelSelector>(),
            detail::MuteOwner(),
            detail::Mute(this->GetMuteControl()),
            ModelAccessors<Model>()
        {
            this->SetWithoutNotify_(plain);
        }

        Model(const Model &) = delete;
        Model(Model &&) = delete;
        Model & operator=(const Model &) = delete;
        Model & operator=(Model &&) = delete;

        bool HasModel() const { return true; }
    };

    template<typename Derived>
    using ControlAccessors = GroupAccessors
        <
            Plain,
            Fields,
            Template,
            ControlSelector,
            Derived
        >;

    struct Control:
        public detail::Mute,
        public Template<ControlSelector>,
        public ControlAccessors<Control>
    {
        static constexpr bool isGroupControl = true;

        using AccessorsBase = ControlAccessors<Control>;

        // using Plain = typename Group::Plain;
        using Type = Plain;

        using Upstream = Model;

        using Defer = DeferGroup
            <
                ControlSelector,
                Control
            >;

        // UpstreamType could be the type returned by a filter.
        // Filters have not been implemented by this class, so it remains the
        // same as the Type.
        using UpstreamType = Plain;
        using Filter = NoFilter;

        static constexpr bool isPexCopyable = true;

        template<typename T>
        using Pex =
            typename pex::ControlSelector<T>;

        Control()
            :
            detail::Mute(),
            AccessorsBase()
        {

        }

        Control(Model &model)
            :
            detail::Mute(model.GetMuteControl()),
            AccessorsBase()
        {
            fields::AssignConvert<Fields>(*this, model);
        }

        Control(const Control &other)
            :
            detail::Mute(other),
            AccessorsBase()
        {
            fields::Assign<Fields>(*this, other);
        }

        Control & operator=(const Control &other)
        {
            this->detail::Mute::operator=(other);
            fields::Assign<Fields>(*this, other);

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

        bool HasModel() const
        {
            return pex::detail::HasModel<Fields>(*this);
        }
    };

    using Aggregate = detail::Aggregate<Plain, Fields, Template_>;

    static typename Model::Defer MakeDefer(Model &model)
    {
        return typename Model::Defer(model);
    }

    static typename Control::Defer MakeDefer(Control &control)
    {
        return typename Control::Defer(control);
    }
};


} // end namespace pex
