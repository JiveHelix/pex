#pragma once


#include <fields/fields.h>
#include <jive/describe_type.h>

#include "pex/identity.h"
#include "pex/selectors.h"
#include "pex/accessors.h"
#include "pex/traits.h"
#include "pex/detail/mute.h"
#include "pex/detail/aggregate.h"
#include "pex/detail/has_model.h"
#include "pex/detail/choose_not_void.h"
#include "pex/detail/traits.h"


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


namespace detail
{


template<typename Custom, typename T, typename = void>
struct CustomizePlain_
{
    using Type = T;
};


template<typename Custom, typename T>
struct CustomizePlain_<Custom, T, std::enable_if_t<HasPlainTemplate<Custom, T>>>
{
    using Type = typename Custom::template Plain<T>;
};


template<typename Custom, typename T>
struct CustomizePlain_
<
    Custom,
    T,
    std::enable_if_t
    <
        HasPlain<Custom> && !HasPlainTemplate<Custom, T>
    >
>
{
    using Type = typename Custom::Plain;
};


template<typename Custom, typename T>
using CustomizePlain = typename CustomizePlain_<Custom, T>::Type;


template<typename Custom, typename T, typename = void>
struct CustomizeModel_
{
    using Type = T;
};


template<typename Custom, typename T>
struct CustomizeModel_<Custom, T, std::enable_if_t<HasModelTemplate<Custom, T>>>
{
    using Type = typename Custom::template Model<T>;
};

template<typename Custom, typename T>
using CustomizeModel = typename CustomizeModel_<Custom, T>::Type;


template<typename Custom, typename T, typename = void>
struct CustomizeControl_
{
    using Type = T;
};

template<typename Custom, typename T>
struct CustomizeControl_
<
    Custom,
    T,
    std::enable_if_t<HasControlTemplate<Custom, T>>
>
{
    using Type = typename Custom::template Control<T>;
};

template<typename Custom, typename T>
using CustomizeControl = typename CustomizeControl_<Custom, T>::Type;


template<typename Custom, typename T, typename = void>
struct CustomizeMux_
{
    using Type = T;
};

template<typename Custom, typename T>
struct CustomizeMux_
<
    Custom,
    T,
    std::enable_if_t<HasMuxTemplate<Custom, T>>
>
{
    using Type = typename Custom::template Mux<T>;
};

template<typename Custom, typename T>
using CustomizeMux = typename CustomizeMux_<Custom, T>::Type;


template<typename Custom, typename T, typename = void>
struct CustomizeFollow_
{
    using Type = T;
};

template<typename Custom, typename T>
struct CustomizeFollow_
<
    Custom,
    T,
    std::enable_if_t<HasFollowTemplate<Custom, T>>
>
{
    using Type = typename Custom::template Follow<T>;
};

template<typename Custom, typename T>
using CustomizeFollow = typename CustomizeFollow_<Custom, T>::Type;



template
<
    typename Custom,
    typename PlainBase,
    typename ModelBase,
    typename ControlBase,
    typename MuxBase,
    typename FollowBase,
    typename = void
>
struct CheckCustom_: std::false_type {};

template
<
    typename Custom,
    typename PlainBase,
    typename ModelBase,
    typename ControlBase,
    typename MuxBase,
    typename FollowBase
>
struct CheckCustom_
<
    Custom,
    PlainBase,
    ModelBase,
    ControlBase,
    MuxBase,
    FollowBase,
    std::enable_if_t
    <
        (
            HasPlainTemplate<Custom, PlainBase>
            || HasPlain<Custom>
            || HasModelTemplate<Custom, ModelBase>
            || HasControlTemplate<Custom, ControlBase>
            || HasMuxTemplate<Custom, MuxBase>
            || HasFollowTemplate<Custom, FollowBase>)
    >
>: std::true_type {};


template
<
    typename Custom,
    typename PlainBase,
    typename ModelBase,
    typename ControlBase,
    typename MuxBase,
    typename FollowBase
>
inline constexpr bool CheckCustom =
    CheckCustom_
    <
        Custom,
        PlainBase,
        ModelBase,
        ControlBase,
        MuxBase,
        FollowBase
    >::value;


} // end namespace detail


template<template<typename> typename T>
struct PlainU
{
    template<typename U>
    using Plain = T<U>;
};


template<typename T>
struct PlainT
{
    using Plain = T;
};


template
<
    template<template<typename> typename> typename Template
>
class MakeControlMembers
    :
    public Template<ControlSelector>
{
protected:
    MakeControlMembers() = default;
};


template
<
    template<template<typename> typename> typename Template
>
class MakeMuxMembers
    :
    public Template<MuxSelector>
{
protected:
    MakeMuxMembers()
        :
        Template<MuxSelector>{}
    {

    }
};


template
<
    template<template<typename> typename> typename Template
>
class MakeFollowMembers
    :
    public Template<FollowSelector>
{
protected:
    MakeFollowMembers() = default;
};


template
<
    template<typename> typename Fields_,
    template<template<typename> typename> typename Template_,
    typename Custom = void
>
struct Group
{
    static constexpr bool isGroup = true;

    template<typename T>
    using Fields = Fields_<T>;

    template<template<typename> typename T>
    using Template = Template_<T>;

    static_assert(
        std::is_void_v<Custom>
            || detail::CheckCustom
                <
                    Custom,
                    Template<pex::Identity>,
                    Template<ModelSelector>,
                    Template<ControlSelector>,
                    Template<MuxSelector>,
                    Template<FollowSelector>
                >,
        "Expected at least one customization");

    using Plain = detail::CustomizePlain<Custom, Template<pex::Identity>>;
    using Type = Plain;

    template<template<typename> typename Selector, typename Upstream>
    using DeferGroup = DeferGroup<Fields, Template, Selector, Upstream>;

    // template<template<typename> typename Selector>
    // using DeferredGroup = detail::DeferredGroup<Fields, Template, Selector>;

    template<typename Derived>
    using ModelAccessors = GroupAccessors
        <
            Plain,
            Fields,
            Template_,
            ModelSelector,
            Derived
        >;

    // struct Control_;

    struct Model_:
        public detail::MuteOwner,
        public detail::MuteControl,
        public Template_<ModelSelector>,
        public ModelAccessors<Model_>
    {
    public:
        using GroupType = Group;
        static constexpr bool isGroupModel = true;

        // using ControlType = typename detail::CustomizeControl<Custom, Control_>;
        using Plain = typename Group::Plain;
        using Type = Plain;
        using Defer = DeferGroup<ModelSelector, Model_>;

        // TODO: Pick one
        template<typename T>
        using Pex = pex::ModelSelector<T>;

        template<typename T>
        using Selector = pex::ModelSelector<T>;

        Model_()
            :
            detail::MuteOwner(),
            detail::MuteControl(this->GetMuteNode()),
            Template<ModelSelector>{},
            ModelAccessors<Model_>{}
        {
            this->SetInitial(Plain{});

            PEX_NAME(fmt::format("{} Model", jive::GetTypeName<Plain>()));

            PEX_NAMES(this);
        }

        Model_(const Plain &plain)
            :
            detail::MuteOwner(),
            detail::MuteControl(this->GetMuteNode()),
            Template<ModelSelector>{},
            ModelAccessors<Model_>{}
        {
            this->SetInitial(plain);

            PEX_NAME(fmt::format("{} Model", jive::GetTypeName<Plain>()));
            PEX_NAMES(this);
        }

        Model_(const Model_ &) = delete;
        Model_(Model_ &&) = delete;
        Model_ & operator=(const Model_ &) = delete;
        Model_ & operator=(Model_ &&) = delete;

        ~Model_()
        {
            CLEAR_PEX_NAMES;
            PEX_CLEAR_NAME(this);
        }

        bool HasModel() const { return true; }
    };

    using Model = typename detail::CustomizeModel<Custom, Model_>;

    template<template<typename> typename Selector>
    using Aggregate = detail::Aggregate<Plain, Fields, Template_, Selector>;

    template<typename Derived>
    using ControlAccessors = GroupAccessors
        <
            Plain,
            Fields,
            Template,
            ControlSelector,
            Derived
        >;

    using ControlMembers = MakeControlMembers<Template_>;

    template<typename Upstream_>
    struct Control_:
        public detail::MuteControl,
        public ControlMembers,
        public ControlAccessors<Control_<Upstream_>>
    {
        using GroupType = Group;
        static constexpr bool isGroupControl = true;

        using Aggregate = typename Group::template Aggregate<ControlSelector>;
        using AccessorsBase = ControlAccessors<Control_>;
        using Type = Plain;
        using Upstream = Upstream_;

        using Defer = DeferGroup
            <
                ControlSelector,
                Control_
            >;

        // UpstreamType could be the type returned by a filter.
        // Filters have not been implemented by this class, so it remains the
        // same as the Type.
        using UpstreamType = Plain;
        using Filter = NoFilter;

        static constexpr bool isPexCopyable = true;

        // TODO: Pick one
        template<typename T>
        using Pex = typename pex::ControlSelector<T>;

        template<typename T>
        using Selector = pex::ControlSelector<T>;

        Control_()
            :
            detail::MuteControl(),
            ControlMembers{},
            AccessorsBase{}
        {
            PEX_NAME(fmt::format("{} Control", jive::GetTypeName<Plain>()));
            PEX_NAMES(this);
        }

        Control_(Upstream &upstream)
            :
            detail::MuteControl(upstream.GetMuteNode()),
            ControlMembers{},
            AccessorsBase{}
        {
            fields::AssignConvert<Fields>(*this, upstream);

            PEX_NAME(fmt::format("{} Control", jive::GetTypeName<Plain>()));
            PEX_NAMES(this);
        }

        Control_(const Control_ &other)
            :
            detail::MuteControl(other),
            ControlMembers{},
            AccessorsBase{}
        {
            fields::Assign<Fields>(*this, other);

            PEX_NAME(fmt::format("{} Control", jive::GetTypeName<Plain>()));
            PEX_NAMES(this);
        }

        Control_ & operator=(const Control_ &other)
        {
            this->detail::MuteControl::operator=(other);
            fields::Assign<Fields>(*this, other);

            return *this;
        }

        Control_(Control_ &&other)
            :
            detail::MuteControl(other),
            ControlMembers{},
            AccessorsBase{}
        {
            fields::MoveAssign<Fields>(*this, std::move(other));

            PEX_NAME(fmt::format("{} Control", jive::GetTypeName<Plain>()));
            PEX_NAMES(this);
        }

        ~Control_()
        {
            CLEAR_PEX_NAMES;
            PEX_CLEAR_NAME(this);
        }

        Control_ & operator=(Control_ &&other)
        {
            this->detail::MuteControl::operator=(other);
            fields::MoveAssign<Fields>(*this, std::move(other));

            return *this;
        }

        bool HasModel() const
        {
            return pex::detail::HasModel<Fields>(*this);
        }
    };


    template<typename Upstream>
    using Control =
        typename detail::CustomizeControl<Custom, Control_<Upstream>>;

    using DefaultControl = Control<Model>;

    template<typename Derived>
    using MuxAccessors = GroupAccessors
        <
            Plain,
            Fields,
            Template,
            MuxSelector,
            Derived
        >;

    using MuxMembers = MakeMuxMembers<Template_>;

    struct Mux_:
        public detail::MuteMux,
        public MuxMembers,
        public MuxAccessors<Mux_>
    {
        using GroupType = Group;
        static constexpr bool isGroupMux = true;
        static constexpr bool isPexCopyable = false;

        // We must use FollowSelector to track changes to Mux values.
        using Aggregate = typename Group::template Aggregate<FollowSelector>;
        using AccessorsBase = MuxAccessors<Mux_>;
        using Type = Plain;
        using Upstream = Model;

        using Defer = DeferGroup
            <
                MuxSelector,
                Mux_
            >;

        // UpstreamType could be the type returned by a filter.
        // Filters have not been implemented by this class, so it remains the
        // same as the Type.
        using UpstreamType = Plain;
        using Filter = NoFilter;

        // TODO: Pick one
        template<typename T>
        using Pex = typename pex::MuxSelector<T>;

        template<typename T>
        using Selector = typename pex::MuxSelector<T>;

        Mux_()
            :
            detail::MuteMux(),
            MuxMembers{},
            AccessorsBase{}
        {
            PEX_NAME(fmt::format("{} Control", jive::GetTypeName<Plain>()));
            PEX_NAMES(this);
        }

        Mux_(Model &model)
            :
            detail::MuteMux(model.GetMuteNode()),
            MuxMembers{},
            AccessorsBase{}
        {
            PEX_NAME(fmt::format("{} Control", jive::GetTypeName<Plain>()));
            PEX_NAMES(this);

            this->ChangeUpstream(model);
        }

        Mux_(const Mux_ &) = delete;

        Mux_ & operator=(const Mux_ &other) = delete;

        Mux_(Mux_ &&other) = delete;

        ~Mux_()
        {
            CLEAR_PEX_NAMES;
            PEX_CLEAR_NAME(this);
        }

        Mux_ & operator=(Mux_ &&other) = delete;

        detail::MuteFollow GetMuteNode()
        {
            return this->CloneMuteNode();
        }

        bool HasModel() const
        {
            return pex::detail::HasModel<Fields>(*this);
        }

        void ChangeUpstream(Upstream &upstream)
        {
            this->detail::MuteMux::ChangeUpstream(upstream.GetMuteNode());

            auto swapper = [this, &upstream](
                const auto &thisField,
                const auto &upstreamField) -> void
            {
                (this->*(thisField.member)).ChangeUpstream(
                    upstream.*(upstreamField.member));
            };

            jive::ZipApply(
                swapper,
                Fields<Mux_>::fields,
                Fields<Upstream>::fields);
        }
    };

    using Mux = typename detail::CustomizeMux<Custom, Mux_>;

    template<typename Derived>
    using FollowAccessors = GroupAccessors
        <
            Plain,
            Fields,
            Template,
            FollowSelector,
            Derived
        >;

    using FollowMembers = MakeFollowMembers<Template_>;

    struct Follow_:
        public detail::MuteFollow,
        public FollowMembers,
        public FollowAccessors<Follow_>
    {
        using GroupType = Group;

        // This structure behaves like a group control.
        static constexpr bool isGroupFollow = true;

        using Aggregate = typename Group::template Aggregate<FollowSelector>;
        using AccessorsBase = FollowAccessors<Follow_>;
        using Type = Plain;
        using Upstream = Mux;

        using Defer = DeferGroup
            <
                FollowSelector,
                Follow_
            >;

        // UpstreamType could be the type returned by a filter.
        // Filters have not been implemented by this class, so it remains the
        // same as the Type.
        using UpstreamType = Plain;
        using Filter = NoFilter;

        static constexpr bool isPexCopyable = true;

        // TODO: Pick one
        template<typename T>
        using Pex = typename pex::FollowSelector<T>;

        template<typename T>
        using Selector = typename pex::FollowSelector<T>;

        Follow_()
            :
            detail::MuteFollow(),
            FollowMembers{},
            AccessorsBase{}
        {
            PEX_NAME(fmt::format("{} Follow", jive::GetTypeName<Plain>()));
            PEX_NAMES(this);
        }

        Follow_(Upstream &upstream)
            :
            detail::MuteFollow(upstream.GetMuteNode()),
            FollowMembers{},
            AccessorsBase{}
        {
            fields::AssignConvert<Fields>(*this, upstream);

            PEX_NAME(fmt::format("{} Follow", jive::GetTypeName<Plain>()));
            PEX_NAMES(this);
        }

        Follow_(const Follow_ &other)
            :
            detail::MuteFollow(other),
            FollowMembers{},
            AccessorsBase{}
        {
            fields::Assign<Fields>(*this, other);

            PEX_NAME(fmt::format("{} Follow", jive::GetTypeName<Plain>()));
            PEX_NAMES(this);
        }

        Follow_ & operator=(const Follow_ &other)
        {
            this->detail::MuteFollow::operator=(other);
            fields::Assign<Fields>(*this, other);

            return *this;
        }

        Follow_(Follow_ &&other)
            :
            detail::MuteFollow(other),
            FollowMembers{},
            AccessorsBase{}
        {
            fields::MoveAssign<Fields>(*this, std::move(other));

            PEX_NAME(fmt::format("{} Follow", jive::GetTypeName<Plain>()));
            PEX_NAMES(this);
        }

        ~Follow_()
        {
            CLEAR_PEX_NAMES;
            PEX_CLEAR_NAME(this);
        }

        Follow_ & operator=(Follow_ &&other)
        {
            this->detail::MuteFollow::operator=(other);
            fields::MoveAssign<Fields>(*this, std::move(other));

            return *this;
        }

        bool HasModel() const
        {
            return pex::detail::HasModel<Fields>(*this);
        }
    };

    using Follow = typename detail::CustomizeFollow<Custom, Follow_>;

    static typename Model::Defer MakeDefer(Model &model)
    {
        return typename Model::Defer(model);
    }

    template<typename Upstream>
    static typename Control<Upstream>::Defer MakeDefer(
        Control<Upstream> &control)
    {
        return typename Control<Upstream>::Defer(control);
    }
};


} // end namespace pex
